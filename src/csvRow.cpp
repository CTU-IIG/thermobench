#include "csvRow.h"

/* CsvColumn implementation */

CsvColumn::CsvColumn(string header, unsigned int order)
    : header(header)
    , order(order)
{
}

string CsvColumn::getHeader() const
{
    return header;
}

unsigned int CsvColumn::getOrder() const
{
    return order;
}

/* CsvColumns implementation */

const CsvColumn &CsvColumns::add(string header)
{
    columns.push_back(CsvColumn(header, columns.size()));
    return columns.back();
}

void CsvColumns::setHeader(CsvRow &row)
{
    for (const CsvColumn &column : columns) {
        row.set(column, column.getHeader());
    }
}

/* CsvRow implementation */
void CsvRow::set(const CsvColumn &column, double data)
{
    char buf[100];
    sprintf(buf, "%g", data);
    set(column, buf);
};

void CsvRow::set(const CsvColumn &column, const string &data)
{
    const unsigned int order = column.getOrder();
    string esc_data = csvEscape(data);
    m_empty = false;
    if (order < row.size()) {
        row[order] = esc_data;
    } else {
        while (row.size() < order) {
            row.push_back("");
        }
        row.push_back(esc_data);
    }
};

string CsvRow::getValue(const CsvColumn &column) const
{
    const unsigned int order = column.getOrder();
    return (order < row.size()) ? row[column.getOrder()] : "";
}

string CsvRow::toString() const
{
    string line;
    for (const string &str : row) {
        line.append(str);
        line.push_back(',');
    }
    line.pop_back(); // remove last ','
    line.push_back('\n');
    return line;
}

void CsvRow::write(FILE *fp)
{
    if (fp)
        fprintf(fp, "%s", (toString()).c_str());
}

void CsvRow::clear()
{
    row.clear();
    row.resize(num_columns);
    m_empty = true;
}

string csvEscape(string unsafe)
{
    // Each of the embedded double-quote characters
    // must be represented by a pair of double-quote characters
    size_t index = 0;
    while ((index = unsafe.find_first_of('"', index)) != string::npos) {
        unsafe.insert(index, "\"");
        index += 2; // after insert index of founded character is incremented, therefore +2
    }
    // Fields with embedded commas, quotes or line breaks characters must be quoted
    if (unsafe.find_first_of(",\"\r\n") != string::npos) {
        unsafe.insert(0, "\"");
        unsafe.push_back('"');
    }
    return unsafe;
}
