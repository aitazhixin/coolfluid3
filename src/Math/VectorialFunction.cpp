// Copyright (C) 2010 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#include <boost/tokenizer.hpp>

#include "Common/Log.hpp"
#include "Common/BasicExceptions.hpp"
#include "Common/String/Conversion.hpp"

#include "Math/VectorialFunction.hpp"

////////////////////////////////////////////////////////////////////////////////

using namespace std;
using namespace CF::Common;

////////////////////////////////////////////////////////////////////////////////

namespace CF {
namespace Math {

////////////////////////////////////////////////////////////////////////////////

VectorialFunction::VectorialFunction()
  : m_is_parsed(false),
    m_vars(""),
    m_nbvars(0),
    m_functions(0),
    m_parsers(),
    m_result()
{
}

VectorialFunction::VectorialFunction( const std::string& funcs, const std::string& vars)
  : m_is_parsed(false),
    m_vars(""),
    m_nbvars(0),
    m_functions(0),
    m_parsers(),
    m_result()
{
  functions( funcs );
  variables( vars );
  parse();
}


////////////////////////////////////////////////////////////////////////////////

VectorialFunction::~VectorialFunction()
{
  clear();
}

////////////////////////////////////////////////////////////////////////////////

void VectorialFunction::clear()
{
  m_is_parsed = false;
  m_result.resize(0);
  for(Uint i = 0; i < m_parsers.size(); i++) {
      delete_ptr(m_parsers[i]);
  }
  vector<FunctionParser*>().swap(m_parsers);
}

////////////////////////////////////////////////////////////////////////////////

Uint VectorialFunction::nbvars() const
{
  cf_assert ( is_parsed() );
  return m_nbvars;
}

Uint VectorialFunction::nbfuncs() const
{
  cf_assert ( is_parsed() );
  return m_functions.size();
}

////////////////////////////////////////////////////////////////////////////////

void VectorialFunction::variables(const vector<std::string>& vars)
{
  m_nbvars = vars.size();
  m_vars = "";
  if(vars.size() > 0) {
    m_vars = vars[0];
    for(Uint i = 1; i < m_nbvars; i++) {
      m_vars += ",";
      m_vars += vars[i];
    }
  }
  m_is_parsed = false;
}

////////////////////////////////////////////////////////////////////////////////

void VectorialFunction::variables(const std::string& vars)
{
  // assume string is comma separated
  m_vars = vars;

  // count them
  boost::char_separator<char> sep(",");
  typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
  tokenizer tok (vars,sep);

  m_nbvars = 0;
  for (tokenizer::iterator el=tok.begin(); el!=tok.end(); ++el, ++m_nbvars )
  {
    CFinfo << "var" << m_nbvars << " [" << *el << "]" << CFendl;
  }

  m_is_parsed = false;
}

////////////////////////////////////////////////////////////////////////////////

void VectorialFunction::functions( const std::string& functions )
{
  m_functions.clear();

  boost::char_separator<char> sep("[]");

  // break path in tokens and loop on them, while concatenaitng to a new path
  typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
  tokenizer tok (functions,sep);

  for (tokenizer::iterator el=tok.begin(); el!=tok.end(); ++el)
  {
    CFinfo << "func  [" << *el << "]" << CFendl;
    m_functions.push_back(*el);
  }

  m_is_parsed = false;
}

////////////////////////////////////////////////////////////////////////////////

void VectorialFunction::functions(const vector<std::string>& functions)
{
  m_functions = functions;
  m_is_parsed = false;
}

////////////////////////////////////////////////////////////////////////////////

void VectorialFunction::parse()
{
  clear();

  for(Uint i = 0; i < m_functions.size(); i++)
  {
    FunctionParser* ptr = new FunctionParser();

    m_parsers.push_back(ptr);
    CFinfo << "Parsing Function: [" << m_functions[i] << "] Vars: [" << m_vars << "]\n" << CFendl;
    ptr->Parse(m_functions[i],m_vars);

    if (ptr->ErrorMsg() != 0)
    {
      std::string msg("ParseError in VectorialFunction::parse(): ");
      msg += std::string(ptr->ErrorMsg());
      msg += " Function [" + m_functions[i] + "]";
      msg += " Vars: ["    + m_vars + "]";
      throw Common::ParsingFailed (FromHere(),msg);
    }
  }

  m_result.resize(m_functions.size());
  m_is_parsed = true;
}

////////////////////////////////////////////////////////////////////////////////

void VectorialFunction::evaluate(const RealVector& var_values, RealVector& value) const
{
  cf_assert(m_is_parsed);
  cf_assert(var_values.size() == m_nbvars);

  // evaluate and store the functions line by line in the vector
  std::vector<FunctionParser*>::const_iterator parser = m_parsers.begin();
  std::vector<FunctionParser*>::const_iterator end = m_parsers.end();
  Uint i = 0;
  for( ; parser != end ; ++parser, ++i )
    value[i] = (*parser)->Eval(var_values.getRawPtr());
}

////////////////////////////////////////////////////////////////////////////////

RealVector& VectorialFunction::operator()(const RealVector& var_values)
{
  cf_assert(m_is_parsed);
  cf_assert(var_values.size() == m_nbvars);

  // evaluate and store the functions line by line in the result vector
  std::vector<FunctionParser*>::const_iterator parser = m_parsers.begin();
  std::vector<FunctionParser*>::const_iterator end = m_parsers.end();
  Uint i = 0;
  for( ; parser != end ; ++parser, ++i )
    m_result[i] = (*parser)->Eval(var_values.getRawPtr());

  return m_result;
}

////////////////////////////////////////////////////////////////////////////////

} // Math
} // CF

////////////////////////////////////////////////////////////////////////////////

