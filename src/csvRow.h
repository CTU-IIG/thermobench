#ifndef CSVROW
#define CSVROW

#include <iostream>
#include <vector>

std::string csvEscape(std::string unsafe);

class CsvRow;

class CsvColumn {
private:
    std::string header;
    unsigned int order;

public:
    CsvColumn(std::string header, unsigned int order);

    std::string getHeader() const;

    unsigned int getOrder() const;
};

class CsvColumns {
private:
    std::vector<CsvColumn> columns;

public:
    const CsvColumn &add(std::string header);

    void setHeader(CsvRow &row);
};

class CsvRow {
private:
    std::vector<std::string> row;

public:
    void set(const CsvColumn &column, double data);
    void set(const CsvColumn &column, std::string data);

    std::string getValue(const CsvColumn &column) const;

    std::string toString() const;

    void write(FILE *fp);

    void clear();

    bool empty() const;
};
#endif
