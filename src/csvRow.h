#ifndef CSVROW
#define CSVROW

#include <iostream>
#include <list>
#include <vector>

using namespace std;

string csvEscape(string unsafe);

class CsvRow;

class CsvColumn {
private:
    string header;
    unsigned int order;

public:
    CsvColumn(string header, unsigned int order);

    string getHeader() const;

    unsigned int getOrder() const;
};

class CsvColumns {
private:
    list<CsvColumn> columns = {};

public:
    const CsvColumn &add(string header);

    void setHeader(CsvRow &row);

    size_t count() const { return columns.size(); }
};

class CsvRow {
private:
    size_t num_columns;
    vector<string> row { num_columns };
    bool m_empty = true;

public:
    CsvRow(const CsvColumns &cols)
        : num_columns(cols.count())
    {
        clear();
    }

    void set(const CsvColumn &column, double data);
    void set(const CsvColumn &column, const string &data);

    string getValue(const CsvColumn &column) const;

    string toString() const;

    void write(FILE *fp);

    void clear();

    bool empty() const { return m_empty; }
};
#endif
