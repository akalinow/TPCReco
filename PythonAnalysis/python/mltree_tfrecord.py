#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Convert the MLData tree written by tpcGRAW2ROOT (--output.mlTree true) to TFRecord.

Writing needs only uproot, awkward, numpy and google-crc32c (TensorFlow is not
needed). Reading (load_dataset) needs TensorFlow.

Command line:

  mltree_tfrecord.py INPUT.root [INPUT.root ...] -o OUTPUT_DIR
                     [--events-per-shard 1000] [--jobs 4]

Each input file gives shards OUTPUT_DIR/<input stem>-00000.tfrecord.gz, ...
(GZIP compressed TFRecord files, one tf.train.Example per event).

Features of each tf.train.Example:

  run_id, event_id, timestamp     int64
  hit_dir, hit_strip, hit_time    int64 list  (strip direction U,V,W = 0,1,2;
                                               strip number; time cell)
  hit_charge                      float list  (charge, strip sections merged)
  sim_pid, reco_pid               int64 list  (one per track segment)
  sim_start, sim_end,             float list  (x,y,z [mm] of each segment,
  reco_start, reco_end                         i.e. 3 values per segment)
  reco_length, reco_loss,         float
  reco_hypothesis_loss

The charge is stored as it is (no normalisation) and sparse: load_dataset
builds the dense (nStrips, nTimeSlices, nProj) image and normalises it.
"""

import argparse
import gzip
import os
import struct
import sys
from multiprocessing import Pool

import numpy as np

## Dense image shape, as in io_functions.py
nStrips = 256
nTimeSlices = 512
nProj = 3

TREE_NAME = "MLData"

SEGMENT_BRANCHES = ["pid", "startX", "startY", "startZ", "endX", "endY", "endZ"]
BRANCHES = (["runId", "eventId", "timestamp",
             "hitDir", "hitStrip", "hitTime", "hitCharge",
             "reco_length", "reco_loss", "reco_hypothesisLoss"]
            + ["sim_" + b for b in SEGMENT_BRANCHES]
            + ["reco_" + b for b in SEGMENT_BRANCHES])

################################################################
## TFRecord writing without TensorFlow
################################################################
try:
    import google_crc32c

    def _crc32c(data):
        return google_crc32c.value(data)
except ImportError:  # slow pure Python fallback
    _CRC_TABLE = []
    for _i in range(256):
        _c = _i
        for _ in range(8):
            _c = (_c >> 1) ^ 0x82F63B78 if _c & 1 else _c >> 1
        _CRC_TABLE.append(_c)

    def _crc32c(data):
        crc = 0xFFFFFFFF
        for byte in data:
            crc = _CRC_TABLE[(crc ^ byte) & 0xFF] ^ (crc >> 8)
        return crc ^ 0xFFFFFFFF


def _masked_crc(data):
    crc = _crc32c(data)
    return (((crc >> 15) | (crc << 17)) + 0xA282EAD8) & 0xFFFFFFFF


def write_record(stream, data):
    """Write one TFRecord: length, CRC of length, data, CRC of data."""
    length = struct.pack("<Q", len(data))
    stream.write(length)
    stream.write(struct.pack("<I", _masked_crc(length)))
    stream.write(data)
    stream.write(struct.pack("<I", _masked_crc(data)))


def _varint(value):
    """Protobuf varint of a non-negative Python int."""
    out = bytearray()
    while True:
        byte = value & 0x7F
        value >>= 7
        if value:
            out.append(byte | 0x80)
        else:
            out.append(byte)
            return bytes(out)


def _varints(values):
    """Concatenated protobuf varints of an integer array (vectorised).

    Negative values are encoded as 10 byte two's complement, as protobuf int64.
    """
    v = np.asarray(values).astype(np.int64).view(np.uint64).copy()
    if v.size == 0:
        return b""
    nbytes = np.ones(v.size, dtype=np.int64)
    rest = v >> np.uint64(7)
    while np.any(rest):
        nbytes += rest > 0
        rest >>= np.uint64(7)
    starts = np.cumsum(nbytes) - nbytes
    out = np.empty(int(nbytes.sum()), dtype=np.uint8)
    for k in range(int(nbytes.max())):
        sel = nbytes > k
        byte = (v[sel] & np.uint64(0x7F)).astype(np.uint8)
        byte |= (nbytes[sel] > k + 1).astype(np.uint8) << 7
        out[starts[sel] + k] = byte
        v[sel] >>= np.uint64(7)
    return out.tobytes()


def _field(number, wire_type, payload=None, value=None):
    """Protobuf field: tag + (length delimited payload | varint value)."""
    tag = _varint((number << 3) | wire_type)
    if wire_type == 2:
        return tag + _varint(len(payload)) + payload
    return tag + _varint(value)


def _int64_feature(values):
    packed = _varints(values)
    int64_list = _field(1, 2, packed) if packed else b""   # Int64List.value (packed)
    return _field(3, 2, int64_list)                        # Feature.int64_list


def _float_feature(values):
    data = np.asarray(values, dtype="<f4").tobytes()
    float_list = _field(1, 2, data) if data else b""       # FloatList.value (packed)
    return _field(2, 2, float_list)                        # Feature.float_list


def make_example(features):
    """Serialised tf.train.Example from {name: (kind, values)}, kind 'int'|'float'."""
    entries = b""
    for name, (kind, values) in features.items():
        feature = _int64_feature(values) if kind == "int" else _float_feature(values)
        entry = _field(1, 2, name.encode()) + _field(2, 2, feature)   # map entry
        entries += _field(1, 2, entry)                                # Features.feature
    return _field(1, 2, entries)                                      # Example.features

################################################################
## ROOT -> TFRecord
################################################################
class _Batch:
    """A batch of the MLData tree as flat numpy arrays and offsets."""

    def __init__(self, arrays):
        import awkward as ak
        self.size = len(arrays)
        self.scalars = {name: ak.to_numpy(arrays[name]) for name in
                        ["runId", "eventId", "timestamp",
                         "reco_length", "reco_loss", "reco_hypothesisLoss"]}
        self.flat, self.offsets = {}, {}
        for name in BRANCHES:
            if name in self.scalars:
                continue
            self.flat[name] = ak.to_numpy(ak.flatten(arrays[name]))
            counts = ak.to_numpy(ak.num(arrays[name]))
            self.offsets[name] = np.concatenate([[0], np.cumsum(counts)])

    def jagged(self, name, i):
        return self.flat[name][self.offsets[name][i]:self.offsets[name][i + 1]]

    def xyz(self, prefix, i):
        """x,y,z of each segment, flattened: [x0,y0,z0,x1,y1,z1,...]."""
        return np.stack([self.jagged(prefix + c, i) for c in "XYZ"], axis=-1).reshape(-1)

    def features(self, i):
        """Features of the i-th event of the batch."""
        s = self.scalars
        return {
            "run_id": ("int", [s["runId"][i]]),
            "event_id": ("int", [s["eventId"][i]]),
            "timestamp": ("int", [s["timestamp"][i]]),
            "hit_dir": ("int", self.jagged("hitDir", i)),
            "hit_strip": ("int", self.jagged("hitStrip", i)),
            "hit_time": ("int", self.jagged("hitTime", i)),
            "hit_charge": ("float", self.jagged("hitCharge", i)),
            "sim_pid": ("int", self.jagged("sim_pid", i)),
            "sim_start": ("float", self.xyz("sim_start", i)),
            "sim_end": ("float", self.xyz("sim_end", i)),
            "reco_pid": ("int", self.jagged("reco_pid", i)),
            "reco_start": ("float", self.xyz("reco_start", i)),
            "reco_end": ("float", self.xyz("reco_end", i)),
            "reco_length": ("float", [s["reco_length"][i]]),
            "reco_loss": ("float", [s["reco_loss"][i]]),
            "reco_hypothesis_loss": ("float", [s["reco_hypothesisLoss"][i]]),
        }


def convert_file(rootFile, outputDir, eventsPerShard=1000, stepSize=1000, treeName=TREE_NAME):
    """Convert one ROOT file. Returns (number of events, list of shard paths)."""
    import uproot

    stem = os.path.splitext(os.path.basename(rootFile))[0]
    os.makedirs(outputDir, exist_ok=True)
    shards, stream = [], None
    nEvents = 0
    for arrays in uproot.iterate(rootFile + ":" + treeName, BRANCHES,
                                 step_size=stepSize, library="ak"):
        batch = _Batch(arrays)
        for i in range(batch.size):
            if nEvents % eventsPerShard == 0:
                if stream:
                    stream.close()
                path = os.path.join(outputDir, "{}-{:05d}.tfrecord.gz".format(
                    stem, nEvents // eventsPerShard))
                stream = gzip.open(path, "wb")
                shards.append(path)
            write_record(stream, make_example(batch.features(i)))
            nEvents += 1
    if stream:
        stream.close()
    return nEvents, shards


def _convert_job(args):
    rootFile, outputDir, eventsPerShard, stepSize = args
    nEvents, shards = convert_file(rootFile, outputDir, eventsPerShard, stepSize)
    return rootFile, nEvents, len(shards)

################################################################
## TFRecord -> tf.data.Dataset (needs TensorFlow)
################################################################
def _feature_spec():
    import tensorflow as tf
    spec = {name: tf.io.FixedLenFeature([], tf.int64)
            for name in ["run_id", "event_id", "timestamp"]}
    spec.update({name: tf.io.FixedLenFeature([], tf.float32)
                 for name in ["reco_length", "reco_loss", "reco_hypothesis_loss"]})
    spec.update({name: tf.io.VarLenFeature(tf.int64)
                 for name in ["hit_dir", "hit_strip", "hit_time", "sim_pid", "reco_pid"]})
    spec.update({name: tf.io.VarLenFeature(tf.float32)
                 for name in ["hit_charge", "sim_start", "sim_end", "reco_start", "reco_end"]})
    return spec


def _dense_image(event):
    """(nStrips, nTimeSlices, nProj) charge image, index [strip, time cell, direction]."""
    import tensorflow as tf
    strip = tf.sparse.to_dense(event["hit_strip"])
    time = tf.sparse.to_dense(event["hit_time"])
    direction = tf.sparse.to_dense(event["hit_dir"])
    charge = tf.sparse.to_dense(event["hit_charge"])
    inside = (strip < nStrips) & (time < nTimeSlices) & (direction < nProj)
    indices = tf.stack([tf.boolean_mask(strip, inside), tf.boolean_mask(time, inside),
                        tf.boolean_mask(direction, inside)], axis=1)
    return tf.scatter_nd(indices, tf.boolean_mask(charge, inside),
                         shape=[nStrips, nTimeSlices, nProj])


def _target(event, prefix, n_segments):
    """(3, 1 + n_segments) target: column 0 = start of the first segment (vertex),
    column 1+i = end of segment i; rows = x, y, z. Missing segments are filled
    with zeros, segments beyond n_segments are dropped.
    For n_segments=2 this is the (3, 3) target of io_functions.generator."""
    import tensorflow as tf
    start = tf.reshape(tf.sparse.to_dense(event[prefix + "_start"]), [-1, 3])
    end = tf.reshape(tf.sparse.to_dense(event[prefix + "_end"]), [-1, 3])
    start = tf.pad(start, [[0, tf.maximum(1 - tf.shape(start)[0], 0)], [0, 0]])
    end = tf.pad(end, [[0, tf.maximum(n_segments - tf.shape(end)[0], 0)], [0, 0]])
    points = tf.concat([start[:1], end[:n_segments]], axis=0)   # (1 + n_segments, 3)
    return tf.ensure_shape(tf.transpose(points), [3, 1 + n_segments])


def tf_size(sparse):
    """Number of values of a sparse (VarLenFeature) tensor, as int64."""
    import tensorflow as tf
    return tf.cast(tf.size(sparse.values), tf.int64)


def _records(files, parallel_reads=False):
    """TFRecordDataset of the shards: a path, a glob pattern or a list of them."""
    import tensorflow as tf
    if isinstance(files, str):
        files = [files]
    paths = []
    for f in files:
        paths += tf.io.gfile.glob(f) if any(c in f for c in "*?[") else [f]
    if not paths:
        raise FileNotFoundError("No TFRecord files: {}".format(files))
    return tf.data.TFRecordDataset(sorted(paths), compression_type="GZIP",
                                   num_parallel_reads=tf.data.AUTOTUNE if parallel_reads else None)


def _normalised(image, normalise):
    import tensorflow as tf
    if normalise == "max":
        maxValue = tf.reduce_max(image)
        return image / tf.where(maxValue > 0, maxValue, 1.0)
    if normalise is not None:
        return image / float(normalise)
    return image


def load_dataset(files, normalise="max", target="sim", keep_info=False,
                 parallel_reads=False, n_segments=3):
    """tf.data.Dataset from TFRecord shards made by convert_file.

    files      -- shard path, glob pattern or list of them
    normalise  -- "max": divide each event by its maximal charge (as io_functions.py),
                  None: raw charge, a number: divide by this number
    target     -- "sim", "reco" or None: (3, 1 + n_segments) target from simulated
                  or reconstructed segments (rows x,y,z; columns vertex and segment
                  ends); None gives the image only
    keep_info  -- if True, elements are dicts with the image, target, the numbers
                  of segments (n_sim_segments, n_reco_segments), the segment pids
                  (sim_pid, reco_pid, padded with 0 to n_segments) and all scalar
                  features (run_id, event_id, ...)
    n_segments -- maximal number of segments in the target: 3 for two and
                  three prong events, 2 for the (3, 3) target of io_functions.py
    parallel_reads -- read shards in parallel (faster, but events from different
                  shards are interleaved); by default shards are read one by one,
                  in sorted order
    """
    import tensorflow as tf
    spec = _feature_spec()

    def parse(record):
        event = tf.io.parse_single_example(record, spec)
        image = _normalised(_dense_image(event), normalise)
        result = image if target is None else (image, _target(event, target, n_segments))
        if keep_info:
            info = {name: event[name] for name in ["run_id", "event_id", "timestamp",
                                                   "reco_length", "reco_loss",
                                                   "reco_hypothesis_loss"]}
            for prefix in ["sim", "reco"]:
                pid = tf.sparse.to_dense(event[prefix + "_pid"])[:n_segments]
                info[prefix + "_pid"] = tf.pad(pid, [[0, n_segments - tf.shape(pid)[0]]])
                info["n_" + prefix + "_segments"] = tf_size(event[prefix + "_pid"])
            info["image"] = image
            if target is not None:
                info["target"] = result[1]
            return info
        return result

    return _records(files, parallel_reads).map(parse, num_parallel_calls=tf.data.AUTOTUNE,
                                                deterministic=not parallel_reads)

def load_merged_dataset(files, normalise="max", n_segments=2, parallel_reads=False):
    """tf.data.Dataset in the format of the merged datasets made by the old
    ROOT_to_TFRecord.ipynb (tf.data.Dataset.save): elements are

        {'sim': (image, simTarget), 'reco': (image, recoTarget)}

    with the (3, 1 + n_segments) targets of load_dataset (n_segments=2: the
    (3, 3) target used by utility_functions and the WAWTPC_ML models).
    For real data (no simulated segments) 'sim' holds the reconstructed
    target, as the old notebook did. Other arguments as for load_dataset.
    """
    import tensorflow as tf
    spec = _feature_spec()

    def parse(record):
        event = tf.io.parse_single_example(record, spec)
        image = _normalised(_dense_image(event), normalise)
        recoTarget = _target(event, "reco", n_segments)
        simTarget = tf.where(tf_size(event["sim_pid"]) > 0,
                             _target(event, "sim", n_segments), recoTarget)
        return {"sim": (image, simTarget), "reco": (image, recoTarget)}

    return _records(files, parallel_reads).map(parse, num_parallel_calls=tf.data.AUTOTUNE,
                                                deterministic=not parallel_reads)


def segment_length(target, iSegment=0):
    """Length of segment iSegment from a (3, 1 + n_segments) target: distance
    between the vertex (column 0) and the segment end (column 1 + iSegment).
    E.g. alpha track length of two prong events: segment_length(x['sim'][1], 0)."""
    import tensorflow as tf
    return tf.norm(target[:, 1 + iSegment] - target[:, 0])

################################################################
################################################################
def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("inputs", nargs="+", help="ROOT files with the MLData tree")
    parser.add_argument("-o", "--output", required=True, help="output directory")
    parser.add_argument("--events-per-shard", type=int, default=1000)
    parser.add_argument("--step", type=int, default=1000,
                        help="number of events read from ROOT at once")
    parser.add_argument("--jobs", type=int, default=1, help="files converted in parallel")
    args = parser.parse_args(argv)

    jobs = [(f, args.output, args.events_per_shard, args.step) for f in args.inputs]
    if args.jobs > 1 and len(jobs) > 1:
        with Pool(args.jobs) as pool:
            results = pool.map(_convert_job, jobs)
    else:
        results = [_convert_job(job) for job in jobs]
    for rootFile, nEvents, nShards in results:
        print("{}: {} events in {} shard(s)".format(rootFile, nEvents, nShards))
    return 0


if __name__ == "__main__":
    sys.exit(main())
