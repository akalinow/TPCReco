#ifndef TPCSOFT_TABULARSTREAM_H
#define TPCSOFT_TABULARSTREAM_H

#include <string>
#include <iostream>
#include <vector>
#include <deque>
#include <sstream>



namespace utl {


    /// Holds TabularStream cell entry

    class TableCell {
    public:
        TableCell() { }
        TableCell(const TableCell& c) : fEntry(c.fEntry.str()) { }
        TableCell& operator=(const TableCell& c)
        { fEntry.str(c.fEntry.str()); return *this; }

    private:
        enum EJustification {
            eFlushLeft,
            eFlushRight,
            eCenter,
            eCharAligned
        };

        template<typename T>
        TableCell& operator<<(const T& item) { fEntry << item; return *this; }

        std::string GetEntry(const unsigned int left = 0, const unsigned int right = 0) const
        { return std::string(left, ' ') + fEntry.str() + std::string(right, ' '); }

        std::string GetEntry(const EJustification just, const int totalSize,
                             const int leftSize = 0, const char mid = '\0') const;

        std::size_t GetSize() const { return fEntry.str().size(); }

        std::pair<int, int> GetSize(const char mid) const;

        std::ostringstream fEntry;

        friend class TableColumn;
        friend class TabularStream;
    };


    // Holds one column of TableCells in the TabularStream class

    class TableColumn {
    public:
        TableColumn()
                : fJustification(TableCell::eFlushLeft), fMidChar('\0'), fWidth(-1), fLeftWidth(0) { }

    private:
        template<typename T>
        TableColumn& operator<<(const T& junk)
        { Front() << junk; return *this; }

        std::size_t GetLength() const { return fCells.size(); }

        void SetMargins(const std::string& prefix,
                        const std::string& postfix = std::string())
        { fPrefix = prefix; fPostfix = postfix; }

        void SetJustification(const TableCell::EJustification just, const char mid = '\0')
        { fJustification = just; fMidChar = mid; }

        std::string Pop();

        void PopFront() { if (!fCells.empty()) fCells.pop_front(); }

        TableCell& Front() { return fCells.front(); }

        void Push() { fCells.push_front(TableCell()); }

        bool Empty() { return fCells.empty(); }

        void CalculateWidths();

        std::string fPrefix;
        std::string fPostfix;
        TableCell::EJustification fJustification;
        char fMidChar;
        std::deque<TableCell> fCells;
        int fWidth;
        int fLeftWidth;

        friend class TabularStream;
    };


    /// type-only supplying class that triggers end-of-column in the TabularStream

    class EndColumn {
    };

    const EndColumn endc = EndColumn();


    /// type-only supplying class that triggers end-of-row in the TabularStream

    class EndRow {
    };

    const EndRow endr = EndRow();

    /** type-only supplying class that triggers end-of-row or -column in the TabularStream
        Useful for loops over columns
    */

    class EndColumnOrRow {
    };

    const EndColumnOrRow endcr = EndColumnOrRow();


    /** type-only supplying class that triggers deleting of the last row in TabularStream
        Useful when in for-loops one too many endr (end-row) is pushed into the
        TabularStream class
    */

    class DeleteRow {
    };

    const DeleteRow delr = DeleteRow();

    /// class that triggers insertion of the line row in the TabularStream

    class HLine {
    public:
        HLine(const char sign) : fChar(sign) { }
        char GetChar() const { return fChar; }
    private:
        char fChar;
    };

    const HLine hline('-');


    /**
      * \class TabularStream TabularStream.h "utl/TabularStream.h"
      *
      * \brief class to format data in tabular form
      *
      * \code
      * TabularStream tab(". r l");
      *
      * tab << 3 << ".14" << endc << "bla"    << endc << 1111U << endr
      *     <<     13.07  << endc << "foobar" << endc <<    22 << endr
      *     <<    123.456 << endc << 'a'      << endc <<   333;
      *
      * cout << tab;
      * \endcode
      * gives you
      * \code
      *   3.14     bla 1111
      *  13.07  foobar 22
      * 123.456      a 333
      * \endcode
      * Use "endc" to end filling the current cell, use "endr" to end a row.
      * Use "endcr" to either end a cell or end the row if the number of cells
      * is equal to the one given in the format (useful for loops over columns).
      * Filling the TabularStream in a for-loop over rows will produce one "endr"
      * too many. Use "delr" to delete the last row (useful if one line too many
      * is added in loops).
      *  More lines around columns or rows you can add
      * as in the following example:
      * \code
      * TabularStream tab("|l|.|r|");
      *
      * tab << hline
      *     << "gain"   << endc <<  1.23 << endc <<  "ok" << endr
      *     << HLine('=')
      *     << "jitter" << endc <<  0.2  << endc <<  "+1" << endr
      *     << "adc"    << endc << 13    << endc << "aha" << endr
      *     << hline;
      *
      * cout << tab;
      * \endcode
      * which produces
      * \code
      * +------+-----+---+
      * |gain  | 1.23| ok|
      * ==================
      * |jitter| 0.2 | +1|
      * |adc   |13   |aha|
      * +------+-----+---+
      * \endcode
      * Note that every row of the table will be terminated with a
      * newline and leading whitespaces in the format specifier are
      * interpreted as indentation of the table.
      *
      * In short, "endc" has the same meaning as "&" in LaTeX tabular environment,
      * "endr" behaves as LaTeX newline "\\", and "hline" even has the same name...
      *
      * \author Darko Veberic
      * \date 14 Aug 2007
      * \version $Id$
      */

    class TabularStream {
    public:
        TabularStream(const std::string& format)
        { Clear(format); }

        void Clear() { fColumns.clear(); fMakeNewRow = true; }

        void Clear(const std::string& format)
        { Clear(); MakeFormat(format); }

        template<typename T>
        TabularStream&
        operator<<(const T& junk)
        {
            CheckFirst();
            *fCurrentColumn << junk;
            return *this;
        }

        TabularStream& operator<<(const EndColumn)
        { CheckFirst(); NextColumn(); return *this; }

        TabularStream& operator<<(const EndRow)
        { CheckFirst(); NextRow(); return *this; }

        TabularStream&
        operator<<(const EndColumnOrRow)
        {
            CheckFirst();
            if (fCurrentColumn+1 == fColumns.end())
                NextRow();
            else
                NextColumn();
            return *this;
        }

        TabularStream&
        operator<<(const HLine hl)
        {
            CheckFirst();
            if (fCurrentColumn != fColumns.begin())
                *this << "ERROR: hline can be used only at the begining of a new line";
            else {
                for (Columns::iterator cIt = fColumns.begin();
                     cIt != fColumns.end(); ++cIt)
                    *cIt << ((std::string("\r") + hl.GetChar()));
            }
            fMakeNewRow = true;
            return *this;
        }

        TabularStream&
        operator<<(const DeleteRow)
        {
            for (Columns::iterator cIt = fColumns.begin();
                 cIt != fColumns.end(); ++cIt)
                cIt->PopFront();
            fMakeNewRow = true;
            return *this;
        }

        std::string Str();

    private:
        void MakeFormat(const std::string& format);

        void
        CheckFirst()
        {
            if (fMakeNewRow) {
                NextRow();
                fMakeNewRow = false;
            }
        }

        void
        NextColumn()
        {
            ++fCurrentColumn;
            if (fCurrentColumn == fColumns.end()) {
                std::cerr<<"Running out of columns, will fill the last one!"<<std::endl;
                --fCurrentColumn;
            }
        }

        void
        NextRow()
        {
            for (Columns::iterator cIt = fColumns.begin();
                 cIt != fColumns.end(); ++cIt)
                cIt->Push();
            fCurrentColumn = fColumns.begin();
        }

        typedef std::vector<TableColumn> Columns;
        Columns::iterator fCurrentColumn;
        Columns fColumns;
        bool fMakeNewRow;
        unsigned int fNIndentation;
    };


    inline
    std::ostream&
    operator<<(std::ostream& os, TabularStream& ts)
    {
        return os << ts.Str();
    }

}

#endif //TPCSOFT_TABULARSTREAM_H
