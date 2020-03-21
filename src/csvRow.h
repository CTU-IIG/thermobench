#ifndef CSVROW
#define CSVROW

#include <iostream>
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
    vector<CsvColumn> columns;

public:
    const CsvColumn &add(string header);

    void setHeader(CsvRow &row);
};

class CsvRow {
private:
    vector<string> row;

public:
    void set(const CsvColumn &column, double data);
    void set(const CsvColumn &column, string data);

    string getValue(const CsvColumn &column) const;

    string toString() const;

    void write(FILE *fp);

    void clear();

    bool empty() const;
};
#endif
