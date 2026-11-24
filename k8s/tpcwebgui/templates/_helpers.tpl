{{- define "tpcwebgui.name" -}}
{{- default .Chart.Name .Values.nameOverride | trunc 63 | trimSuffix "-" }}
{{- end }}

{{- define "tpcwebgui.fullname" -}}
{{- if .Values.fullnameOverride }}
{{- .Values.fullnameOverride | trunc 63 | trimSuffix "-" }}
{{- else }}
{{- $name := default .Chart.Name .Values.nameOverride }}
{{- if contains $name .Release.Name }}
{{- .Release.Name | trunc 63 | trimSuffix "-" }}
{{- else }}
{{- printf "%s-%s" .Release.Name $name | trunc 63 | trimSuffix "-" }}
{{- end }}
{{- end }}
{{- end }}

{{- define "tpcwebgui.labels" -}}
helm.sh/chart: {{ printf "%s-%s" .Chart.Name .Chart.Version }}
{{ include "tpcwebgui.selectorLabels" . }}
app.kubernetes.io/version: {{ .Chart.AppVersion | quote }}
app.kubernetes.io/managed-by: {{ .Release.Service }}
{{- end }}

{{- define "tpcwebgui.selectorLabels" -}}
app.kubernetes.io/name: {{ include "tpcwebgui.name" . }}
app.kubernetes.io/instance: {{ .Release.Name }}
{{- end }}

{{/* URL prefix: "" or "/path" without trailing slash */}}
{{- define "tpcwebgui.urlPrefix" -}}
{{- $p := trimSuffix "/" (default "" .Values.urlPrefix) }}
{{- if and $p (not (hasPrefix "/" $p)) }}{{ $p = printf "/%s" $p }}{{ end }}
{{- $p }}
{{- end }}

{{- define "tpcwebgui.tokenSecret" -}}
{{- default (printf "%s-token" (include "tpcwebgui.fullname" .)) .Values.accessToken.existingSecret }}
{{- end }}

{{- define "tpcwebgui.pvcName" -}}
{{- default (printf "%s-work" (include "tpcwebgui.fullname" .)) .Values.persistence.existingClaim }}
{{- end }}

{{/* storage class name of the static hostPath volume (no StorageClass object needed) */}}
{{- define "tpcwebgui.hostStorageClass" -}}
{{- printf "%s-host" (include "tpcwebgui.fullname" .) }}
{{- end }}

{{/* TPCReco installation in the pod: <mountPath>/install when built by the init container */}}
{{- define "tpcwebgui.installPath" -}}
{{- if .Values.tpcreco.build.enabled }}{{ printf "%s/install" .Values.tpcreco.mountPath }}{{ else }}{{ .Values.tpcreco.mountPath }}{{ end }}
{{- end }}
