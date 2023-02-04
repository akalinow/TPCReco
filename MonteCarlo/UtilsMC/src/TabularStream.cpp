#include <map>
#include <algorithm>
#include <iterator>
#include <TabularStream.h>

using namespace utl;
using namespace std;


string
TableCell::GetEntry(const EJustification just, const int totalSize,
                    const int leftSize, const char mid)
const
{
    const int size = GetSize();
    const int missing = totalSize - size;

    switch (just) {
        case eFlushLeft:
            return GetEntry(0, missing);
        case eFlushRight:
            return GetEntry(missing);
        case eCenter:
        {
            const int half = missing/2;
            return GetEntry(missing - half, half);
        }
        case eCharAligned:
        {
            const size_t pos = fEntry.str().find_first_of(mid);
            const bool hasChar = (pos != string::npos);
            if (hasChar) {
                const int leftPad = leftSize - pos;
                const int rightPad = totalSize - (leftPad + size);
                return GetEntry(leftPad, rightPad);
            } else {
                const string& str = fEntry.str();
                const bool isAlpha =
                        (str.find_first_not_of(" +-0123456789eE.") != string::npos);
                if (isAlpha)
                    return GetEntry(eCenter, totalSize);
                else {
                    const size_t posE = str.find_first_of("eE");
                    bool hasE = (posE != string::npos);
                    if (hasE)
                        return GetEntry(just, totalSize, leftSize, str.at(posE));
                    else {
                        const int leftPad = leftSize - size;
                        const int rightPad = totalSize - leftSize;
                        return GetEntry(leftPad, rightPad);
                    }
                }
            }
        }
        default:
            return string("TableCell::GetEntry ERROR");
    }
}


pair<int, int>
TableCell::GetSize(const char mid)
const
{
    const int size = GetSize();
    const size_t pos = fEntry.str().find_first_of(mid);
    if (pos != string::npos)
        return make_pair(size, pos);
    else {
        // is it a number?
        const bool isAlpha =
                (fEntry.str().find_first_not_of(" +-0123456789eE.") != string::npos);
        if (isAlpha)
            return make_pair(size, size/2);
        else {
            const size_t posE = fEntry.str().find_first_of("eE");
            bool hasE = (posE != string::npos);
            if (hasE)
                return GetSize(fEntry.str().at(posE));
            else
                return make_pair(size, size);
        }
    }
}


string
TableColumn::Pop()
{
    if (fWidth < 0)
        CalculateWidths();

    TableCell& c = fCells.back();
    string res;
    const string entry = c.GetEntry();
    if (entry.size() && entry.at(0) == '\r') {
        const char symbol = entry.at(1);
        if (symbol != '-')
            res = string(fWidth + fPrefix.size() + fPostfix.size(), symbol);
        else {
            for (unsigned int i = 0; i < fPrefix.size(); ++i)
                res += (fPrefix.at(i) == '|') ? '+' : symbol;
            res += string(fWidth, symbol);
            for (unsigned int i = 0; i < fPostfix.size(); ++i)
                res += (fPostfix.at(i) == '|') ? '+' : symbol;
        }
    } else
        res = fPrefix +
              (fJustification == TableCell::eCharAligned ?
               fCells.back().GetEntry(fJustification, fWidth, fLeftWidth, fMidChar) :
               fCells.back().GetEntry(fJustification, fWidth)) +
              fPostfix;
    fCells.pop_back();
    return res;
}


void
TableColumn::CalculateWidths()
{
    fWidth = 0;
    if (fJustification != TableCell::eCharAligned)
        for (deque<TableCell>::const_iterator cIt = fCells.begin();
             cIt != fCells.end(); ++cIt) {
            const int size = cIt->GetSize();
            if (size > fWidth)
                fWidth = size;
        }
    else {
        fLeftWidth = 0;
        int rightWidth = 0;
        for (deque<TableCell>::const_iterator cIt = fCells.begin();
             cIt != fCells.end(); ++cIt) {
            const pair<int, int> size = cIt->GetSize(fMidChar);
            const int right = size.first - size.second;
            if (right > rightWidth)
                rightWidth = right;
            if (size.second > fLeftWidth)
                fLeftWidth = size.second;
        }
        fWidth = fLeftWidth + rightWidth;
    }
}


void
TabularStream::MakeFormat(const string& format)
{

    fNIndentation = format.find_first_not_of(" ");
    string trimmedFormat = format.substr(fNIndentation, format.size());

    map<char, TableCell::EJustification> justMap;
    justMap['l'] = TableCell::eFlushLeft;
    justMap['c'] = TableCell::eCenter;
    justMap['r'] = TableCell::eFlushRight;
    justMap['.'] = TableCell::eCharAligned;

    const int n = trimmedFormat.size();
    string str;
    string pstr;
    for (int i = 0; i < n; ++i) {
        const char c = trimmedFormat.at(i);
        switch (c) {
            case 'l':
            case 'c':
            case 'r':
            case '.':
                fColumns.push_back(TableColumn());
                if (c == '.')
                    fColumns.back().SetJustification(justMap[c], c);
                else
                    fColumns.back().SetJustification(justMap[c]);
                fColumns.back().SetMargins(str);
                pstr = str;
                str = "";
                break;
            default:
                str += c;
                break;
        }
    }
    fColumns.back().SetMargins(pstr, str);
}


string
TabularStream::Str()
{
    ostringstream os;
    const int nRows = fColumns.front().GetLength();
    for (int i = 0; i < nRows; ++i) {
        os << string(fNIndentation, ' ');
        for (Columns::iterator cIt = fColumns.begin();
             cIt != fColumns.end(); ++cIt)
            os << cIt->Pop();
        os << '\n';
    }

    return os.str();
}
