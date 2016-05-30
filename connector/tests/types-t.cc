/*
* Copyright (c) 2015, 2016, Oracle and/or its affiliates. All rights reserved.
*
* The MySQL Connector/C++ is licensed under the terms of the GPLv2
* <http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>, like most
* MySQL Connectors. There are special exceptions to the terms and
* conditions of the GPLv2 as it is applied to this software, see the
* FLOSS License Exception
* <http://www.mysql.com/about/legal/licensing/foss-exception.html>.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published
* by the Free Software Foundation; version 2 of the License.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
* for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include <test.h>
#include <mysqlx.h>
#include <iostream>
#include <array>
#include <cmath>  // for fabs()

using std::cout;
using std::endl;
using std::array;
using std::fabs;
using namespace mysqlx;

class Types : public mysqlx::test::Xplugin
{
};

TEST_F(Types, numeric)
{
  {
    Value val = 7U;
    EXPECT_EQ(Value::UINT64, val.getType());

    int v0;
    EXPECT_NO_THROW(v0 = val);
    EXPECT_EQ(7, v0);

    unsigned v1;
    EXPECT_NO_THROW(v1 = val);
    EXPECT_EQ(7, v1);

    float v2;
    EXPECT_NO_THROW(v2 = val);
    EXPECT_EQ(7, v2);

    double v3;
    EXPECT_NO_THROW(v3 = val);
    EXPECT_EQ(7, v3);

    bool v4;
    EXPECT_NO_THROW(v4 = val);
    EXPECT_TRUE(v4);
  }

  {
    Value val = -7;
    EXPECT_EQ(Value::INT64, val.getType());

    int v0;
    EXPECT_NO_THROW(v0 = val);
    EXPECT_EQ(-7, v0);

    unsigned v1;
    EXPECT_THROW(v1 = val, Error);

    float v2;
    EXPECT_NO_THROW(v2 = val);
    EXPECT_EQ(-7, v2);

    double v3;
    EXPECT_NO_THROW(v3 = val);
    EXPECT_EQ(-7, v3);

    bool v4;
    EXPECT_NO_THROW(v4 = val);
    EXPECT_TRUE(v4);
  }

  {
    unsigned max_uint = std::numeric_limits<unsigned>::max();
    Value val = max_uint;
    EXPECT_EQ(Value::UINT64, val.getType());

    int v0;
    EXPECT_THROW(v0 = val, Error);

    unsigned v1;
    EXPECT_NO_THROW(v1 = val);
    EXPECT_EQ(max_uint, v1);

    float v2;
    EXPECT_NO_THROW(v2 = val);
    // Note: allow small rounding errors
    EXPECT_LE(fabs(v2/max_uint - 1), 1e-7);

    double v3;
    EXPECT_NO_THROW(v3 = val);
    EXPECT_EQ(max_uint, v3);

    bool v4;
    EXPECT_NO_THROW(v4 = val);
    EXPECT_TRUE(v4);
  }

  {
    Value val = 7.0F;
    EXPECT_EQ(Value::FLOAT, val.getType());

    int v0;
    EXPECT_THROW(v0 = val, Error);

    unsigned v1;
    EXPECT_THROW(v1 = val, Error);

    float v2;
    EXPECT_NO_THROW(v2 = val);
    EXPECT_EQ(7.0, v2);

    double v3;
    EXPECT_NO_THROW(v3 = val);
    EXPECT_EQ(7.0, v3);

    bool v4;
    EXPECT_THROW(v4 = val, Error);
  }

  {
    Value val = 7.0;
    EXPECT_EQ(Value::DOUBLE, val.getType());

    int v0;
    EXPECT_THROW(v0 = val, Error);

    unsigned v1;
    EXPECT_THROW(v1 = val, Error);

    float v2;
    EXPECT_THROW(v2 = val, Error);

    double v3;
    EXPECT_NO_THROW(v3 = val);
    EXPECT_EQ(7.0, v3);

    bool v4;
    EXPECT_THROW(v4 = val, Error);
  }

  {
    Value val = true;
    EXPECT_EQ(Value::BOOL, val.getType());

    int v0;
    EXPECT_NO_THROW(v0 = val);
    EXPECT_EQ(1, v0);

    unsigned v1;
    EXPECT_NO_THROW(v1 = val);
    EXPECT_EQ(1, v1);

    float v2;
    EXPECT_THROW(v2 = val, Error);

    double v3;
    EXPECT_THROW(v3 = val, Error);

    bool v4;
    EXPECT_NO_THROW(v4 = val);
    EXPECT_TRUE(v4);
  }
}


TEST_F(Types, basic)
{
  SKIP_IF_NO_XPLUGIN;

  cout << "Preparing test.types..." << endl;

  sql("DROP TABLE IF EXISTS test.types");
  sql(
    "CREATE TABLE test.types("
    "  c0 INT,"
    "  c1 DECIMAL,"
    "  c2 FLOAT,"
    "  c3 DOUBLE,"
    "  c4 VARCHAR(32)"
    ")");

  Table types = getSchema("test").getTable("types");

  int data_int[]        = { 7, -7 };
  double data_decimal[] = { 3.14, -2.71 };
  float  data_float[]   = { 3.1415F, -2.7182F };
  double data_double[]  = { 3.141592, -2.718281 };
  string data_string[]  = { "First row", "Second row" };

  Row row(data_int[0], data_decimal[0], data_float[0], data_double[0],
          data_string[0]);

  types.insert()
    .values(row)
    .values(data_int[1], data_decimal[1], data_float[1], data_double[1],
            data_string[1])
    .execute();

  cout << "Table prepared, querying it..." << endl;

  RowResult res = types.select().execute();

  cout << "Query sent, reading rows..." << endl;
  cout << "There are " << res.getColumnCount() << " columns in the result" << endl;

  for (unsigned i = 0; (row = res.fetchOne()); ++i)
  {
    cout << "== next row ==" << endl;
    for (unsigned j = 0; j < res.getColumnCount(); ++j)
    {
      cout << "- col#" << j << ": " << row[j] << endl;
    }

    EXPECT_EQ(Value::INT64, row[0].getType());
    EXPECT_EQ(Value::RAW, row[1].getType());
    EXPECT_EQ(Value::FLOAT, row[2].getType());
    EXPECT_EQ(Value::DOUBLE, row[3].getType());
    EXPECT_EQ(Value::STRING, row[4].getType());

    EXPECT_EQ(data_int[i], (int)row[0]);
    // TODO: chekc decimal value
    EXPECT_EQ(data_float[i], (float)row[2]);
    EXPECT_EQ(data_double[i], (double)row[3]);
    EXPECT_EQ(data_string[i], (string)row[4]);

    EXPECT_GT(row[1].getRawBytes().size(), 1);
    EXPECT_EQ(data_string[i].length(), string(row[4]).length());
  }

  cout << "Testing Boolean value" << endl;

  types.remove().execute();

  Value bv(false);
  types.insert("c0").values(bv).execute();

  res = types.select().execute();

  row = res.fetchOne();
  EXPECT_TRUE(row);

  cout << "value: " << row[0] << endl;
  EXPECT_FALSE(row[0]);

  cout << "Done!" << endl;
}


TEST_F(Types, blob)
{
  SKIP_IF_NO_XPLUGIN;

  cout << "Preparing test.types..." << endl;

  sql("DROP TABLE IF EXISTS test.types");
  sql(
    "CREATE TABLE test.types("
    "  c0 BLOB"
    ")"
  );

  Table types = getSchema("test").getTable("types");

  bytes data((byte*)"foo\0bar",7);

  types.insert().values(data).execute();

  cout << "Table prepared, querying it..." << endl;

  RowResult res = types.select().execute();

  Row row = res.fetchOne();

  cout << "Got a row, checking data..." << endl;

  Value c0 = row[0];

  EXPECT_EQ(Value::RAW, c0.getType());

  const bytes &dd = c0.getRawBytes();

  cout << "Data length: " << dd.size() << endl;
  EXPECT_EQ(data.size(), dd.size());

  for (byte *ptr = data.begin(); ptr < data.end(); ++ptr)
    EXPECT_EQ(*ptr, dd.begin()[ptr- data.begin()]);

  cout << "Data matches!" << endl;
}


TEST_F(Types, json)
{
  SKIP_IF_NO_XPLUGIN;

  cout << "Preparing test.types..." << endl;

  sql("DROP TABLE IF EXISTS test.types");
  sql(
    "CREATE TABLE test.types("
    "  c0 JSON"
    ")"
    );

  Table types = getSchema("test").getTable("types");

  const char *json = "{"
    "\"foo\": 7,"
    "\"arr\": [1, 2, \"string\"],"
    "\"sub\": { \"day\": 20, \"month\": \"Apr\" }"
  "}";

  types.insert().values(json).execute();

  DbDoc doc(json);
  types.insert().values(doc).execute();

  cout << "Table prepared, querying it..." << endl;

  RowResult res = types.select().execute();

  cout << "Got results, checking data..." << endl;

  Row row;
  for (unsigned i = 0; (row = res.fetchOne()); ++i)
  {
    EXPECT_EQ(Value::DOCUMENT, row[0].getType());

    doc = row[0];
    cout << "- document: " << row[0] << endl;

    EXPECT_TRUE(doc.hasField("foo"));
    EXPECT_TRUE(doc.hasField("arr"));
    EXPECT_TRUE(doc.hasField("sub"));

    EXPECT_EQ(Value::INT64, doc["foo"].getType());
    EXPECT_EQ(Value::ARRAY, doc["arr"].getType());
    EXPECT_EQ(Value::DOCUMENT, doc["sub"].getType());

    EXPECT_EQ(7, (int)doc["foo"]);
    EXPECT_EQ(3, doc["arr"].elementCount());
    EXPECT_TRUE(doc["sub"].hasField("day"));
    EXPECT_TRUE(doc["sub"].hasField("month"));
  }

  cout << "Checking JSON array..." << endl;

  types.remove().execute();
  Value arr = { 1, "a", doc };

  types.insert().values(arr).values("[1, \"a\"]").execute();

  cout << "Arrays inserted, querying data..." << endl;

  res = types.select().execute();

  for (unsigned i = 0; (row = res.fetchOne()); ++i)
  {
    /*
      Note: Even though the value we receive is an array, we
      see it as a JSON value and currently we assume that all
      JSON values are documents. This needs to be fixed eventually,
      so that arrays are returned as array values etc.

      For the same reason we can not access non-document JSON
      values through Value interface. Trying to store row[0]
      in DbDoc will lead to JSON parse errors when DbDoc is accessed.
      Currently we can only see and use the raw JSON string for such
      non-document JSON values.
    */

    EXPECT_EQ(Value::DOCUMENT, row[0].getType());
    cout << "- array: " << row.getBytes(0).begin() << endl;
  }


}


TEST_F(Types, datetime)
{
  SKIP_IF_NO_XPLUGIN;

  cout << "Preparing test.types..." << endl;

  sql("DROP TABLE IF EXISTS test.types");
  sql(
    "CREATE TABLE test.types("
    "  c0 DATE,"
    "  c1 TIME,"
    "  c2 DATETIME,"
    "  c3 TIMESTAMP"
    ")"
    );

  Table types = getSchema("test").getTable("types");

  Row data;

  data[0] = "2014-05-11";
  data[1] = "10:40:23.456";
  data[2] = "2014-05-11 10:40";
  data[3] = "2014-05-11 11:35:00.000";

  types.insert().values(data).execute();

  cout << "Table prepared, querying it..." << endl;

  RowResult res = types.select().execute();
  Row row = res.fetchOne();

  EXPECT_TRUE(row);

  cout << "Got a row, checking data..." << endl;

  for (unsigned j = 0; j < res.getColumnCount(); ++j)
  {
    cout << "- col#" << j << ": " << row[j] << endl;
    EXPECT_EQ(Value::RAW, row[j].getType());
  }
}


TEST_F(Types, set_enum)
{
  SKIP_IF_NO_XPLUGIN;

  cout << "Preparing test.types..." << endl;

  sql("DROP TABLE IF EXISTS test.types");
  sql(
    "CREATE TABLE test.types("
    "  c0 SET('a','b','c'),"
    "  c1 ENUM('a','b','c')"
    ")"
    );

  Table types = getSchema("test").getTable("types");

  Row data[2];

  data[0][0] = "a,b,c";
  data[0][1] = "a";

  data[1][0] = ""; // empty set
  data[1][1] = Value(); // NULL value

  TableInsert insert = types.insert();
  for (Row r : data)
    insert.values(r);
  insert.execute();

  cout << "Table prepared, querying it..." << endl;

  RowResult res = types.select().execute();

  cout << "Got result, checking data..." << endl;

  Row row;
  for (unsigned i = 0; (row = res.fetchOne()); ++i)
  {
    cout << "== next row ==" << endl;
    for (unsigned j = 0; j < res.getColumnCount(); ++j)
    {
      cout << "- col#" << j << ": " << row[j] << endl;
      if (Value::VNULL == data[i][j].getType())
        EXPECT_EQ(Value::VNULL, row[j].getType());
      else
        EXPECT_EQ(j == 0 ? Value::RAW : Value::STRING, row[j].getType());
    }
  }
}

// TODO: Test GEOMETRY types
