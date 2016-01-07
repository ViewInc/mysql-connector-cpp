/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
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

#include <mysql/cdk.h>
#include <mysqlx.h>

/**
  @file
  Implementation of DbDoc and related classes.
*/

#include "impl.h"

#include <vector>
#include <sstream>
#include <iomanip>
#include <memory>

using namespace ::mysqlx;

using std::endl;


// Value
// -----


void Value::print(ostream &out) const
{
  switch (m_type)
  {
  case VNULL: out << "<null>"; return;
  case UINT64: out << m_val._uint64_v; return;
  case INT64: out << m_val._int64_v; return;
  case DOUBLE: out << m_val._double_v; return;
  case FLOAT: out << m_val._float_v; return;
  case BOOL: out << (m_val._bool_v ? "true" : "false"); return;
  case STRING: out << (std::string)m_str; return;
  case DOCUMENT: out << m_doc; return;
  case RAW: out << "<" << m_str.size() << " raw bytes>"; return;
  default:  out << "<unknown value>"; return;
  }
}


// DbDoc implementation
// --------------------


DbDoc::DbDoc(const std::string &json)
  : m_impl(std::make_shared<Impl::JSONDoc>(json))
{}

DbDoc::DbDoc(const std::shared_ptr<Impl> &impl)
  : m_impl(impl)
{}


bool DbDoc::hasField(const Field &fld)
{
  return m_impl->has_field(fld);
}


Value DbDoc::operator[](const Field &fld)
{
  return static_cast<const Value&>(m_impl->get(fld));
}


void DbDoc::print(std::ostream &out) const
{
  m_impl->print(out);
}


// JSON document
// -------------


/*
  JSON processor which builds document implementation adding
  key-value pairs to document's map.
*/

struct DbDoc::Impl::Builder
  : public cdk::JSON::Processor
  , public cdk::JSON::Processor::Any_prc
  , public cdk::JSON::Processor::Any_prc::Scalar_prc
{
  Map  &m_map;
  std::unique_ptr<Field> m_key;

public:

  Builder(DbDoc::Impl &doc)
    : m_map(doc.m_map)
  {}

  // JSON processor

  void doc_begin()
  {
    m_map.clear();
  }

  void doc_end()
  {
  }

  cdk::JSON::Processor::Any_prc*
  key_val(const cdk::string &key)
  {
    m_key.reset(new Field(key));
    return this;
  }

  // TODO: handle arrays

  cdk::JSON::Processor::Any_prc::List_prc*
  arr()
  {
    assert(false);
    return NULL;
  }


  std::unique_ptr<Builder> m_doc_builder;

  cdk::JSON::Processor::Any_prc::Doc_prc*
  doc()
  {
    using mysqlx::Value;
    Value &sub = m_map[*m_key];

    // Turn the value to one storing a document.

    sub.m_type = Value::DOCUMENT;
    sub.m_doc.m_impl = std::make_shared<DbDoc::Impl>();

    // Use another builder to build the sub-document.

    m_doc_builder.reset(new Builder(*sub.m_doc.m_impl));
    return m_doc_builder.get();
  }

  cdk::JSON::Processor::Any_prc::Scalar_prc*
  scalar()
  {
    return this;
  }

  // callbacks for scalar values store the value under
  // key given by m_key.

  void str(const cdk::string &val)
  {
    m_map.emplace(*m_key, Value(val));
  }
  void num(uint64_t val)  { m_map.emplace(*m_key, val); }
  void num(int64_t val)   { m_map.emplace(*m_key, val); }
  void num(float val)     { m_map.emplace(*m_key, val); }
  void num(double val)    { m_map.emplace(*m_key, val); }
  void yesno(bool val)    { m_map.emplace(*m_key, val); }

};


void DbDoc::Impl::JSONDoc::prepare()
{
  if (m_parsed)
    return;

  cdk::Codec<cdk::TYPE_DOCUMENT> codec;
  Builder bld(*this);
  codec.from_bytes(cdk::bytes(m_json), bld);
  m_parsed = true;
}


/*
  Iterating over document fields
  ------------------------------
  Iterator functionality is implemented by document implementation
  object in forms of these methos:

  - reset()  - restart iteration form the beginning,
  - next()   - move to next document field,
  - at_end() - true if all fields have been enumerated,
  - get_current_fld() - return current field in the sequence.

  Note: Since document implementation acts as an iterator, only one
  iterator can be used at a time. Creating new iterator will invalidate
  other iterators.

  Note: Iterator takes shared ownership of document implementation
  so it can be used even if original document was destroyed.
*/


DbDoc::Iterator DbDoc::begin()
{
  Iterator it;
  m_impl->reset();
  it.m_impl = m_impl;
  it.m_end = false;
  return std::move(it);
}

DbDoc::Iterator DbDoc::end()
{
  /*
    Iterator that points one-past-the-end-of-sequence has no
    real representation - we simply set m_end flag in it.
  */
  Iterator it;
  it.m_end = true;
  return std::move(it);
}


const Field& DbDoc::Iterator::operator*()
{
  if (m_end)
    throw "dereferencing past-the-end iterator";
  return m_impl->get_current_fld();
}

DbDoc::Iterator& DbDoc::Iterator::operator++()
{
  // only non-end iterator can be incremented.
  if (!m_end)
    m_impl->next();
  return *this;
}

bool DbDoc::Iterator::operator==(const Iterator &other) const
{
  // if this is end iterator, other is equal if it is also end
  // iterator or it is at the end of sequence. And vice-versa.

  if (m_end)
    return other.m_end || other.m_impl->at_end();

  if (other.m_end)
    return m_end || m_impl->at_end();

  // Otherwise two iterators are equal if they use the same
  // document implementation (but such two iterators should not
  // be used at the same time).

  return m_impl == other.m_impl;
}
