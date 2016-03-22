#if 0 // -*- coding: utf-8 -*-
set -x
g++ -W -Wall -ggdb -O2 -o `basename $0 .cc` $0 $*
exit $?
#endif

/*

This is a mime email message parser to be used as a preprocessor
for email classification software.
Copyright © 2003-2004 Jaakko Hyvätti

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software Foundation,
Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

The author may be contacted at:

Email: Jaakko.Hyvatti@iki.fi
URL:   http://www.iki.fi/hyvatti/
Phone: +358 40 5011222




Read an ftp://ftp.rfc-editor.org/in-notes/rfc2045.txt message and try
to normalize the content to 8bit encoding with utf-8 character set.
Additionally, filter headers and remove HTML from message parts.

After this filtering, the email message no more confirms to any
standards, and formatting information is irreversibly lost.  Even the
MIME message structure is potentially corrupted as the encodings are
decoded and message separators may appear inside the data.

This filter is useful for preprocessing messages for content
recognizing spam filters, like crm114.

TODO:
Use IBM ICU-3.0 library (maybe also earlier versions would work)
from http://ibm.com/software/globalization/icu

$Id: normalizemime.cc,v 1.19 2008/05/19 09:13:18 jaakko Exp jaakko $

*/

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>

#include <string.h>
#include <stdlib.h>
#include <regex.h>
#include <assert.h>
#include <ctype.h>
#include <iconv.h>
#include <sysexits.h>
#include <errno.h>

using namespace std;

//static const char *progname = "normalizemime";

class regex {
  regex_t re;

public:
  // construction of regex object precompiles the given regexp
  regex (const string &expr, int cflags = REG_EXTENDED);
  ~regex () { regfree (&re); }

  // execute (match) the regexp with this data
  bool exec (const string &s) const
  {
    return !regexec (&re, s.c_str(), 0, 0, 0);
  }

  // compile and execute at one go
  static bool comexec (const string &expr, const string &s);

  // vector v is appended with the following strings:
  // v[0]: the part of string s before the start of match
  // v[1]: the full matched part of string s
  // v[2]: the part after the match
  // v[3..]: parenthesized parts of the matched regexp.
  // Note these indexes are for initially empty v.
  bool exec (const string &s, vector<string> &v) const;
  static bool comexec (const string &expr, const string &s, vector<string> &v);

  static string escape (const string &s);
};

string
regex::escape (const string &s)
{
  string r;

  for (string::const_iterator c = s.begin(); c != s.end(); ++c)
    switch (*c) {
    case '\\':
    case '.':
    case '*':
    case '+':
    case '?':
    case '[':
    case ']':
    case '^':
    case '$':
    case '|':
    case '(':
    case ')':
    case '{':
    case '}':
      r += string("\\") + *c;
      break;
    default:
      r += *c;
    }
  return r;
}

bool
regex::comexec (const string &expr, const string &s)
{
  regex r (expr);
  return r.exec (s);
}

bool
regex::comexec (const string &expr, const string &s, vector<string> &v)
{
  regex r (expr);
  return r.exec (s, v);
}

regex::regex (const string &expr, int cflags)
{
  int rv;
  if ((rv = regcomp (&re, expr.c_str(), cflags))) {
    char errbuf [1024];
    regerror (rv, &re, errbuf, sizeof (errbuf));
    //cerr << progname << ": /" << expr << "/: regcomp error: (" << rv << ") "
	// << errbuf << endl;
    exit (1);
  }
}

bool
regex::exec (const string &s, vector<string> &v) const
{
  const int nmatch = 100;
  regmatch_t pmatch [nmatch];

  if (regexec (&re, s.c_str(), nmatch, pmatch, 0))
    return false;

  // The first three elements are: pre-match, match, and post-match.
  assert (-1 != pmatch [0].rm_so && -1 != pmatch [0].rm_eo);
  v.push_back (s.substr (0, pmatch [0].rm_so));
  v.push_back (s.substr (pmatch [0].rm_so, pmatch [0].rm_eo-pmatch [0].rm_so));
  v.push_back (s.substr (pmatch [0].rm_eo));

  int skipped = 0;
  for (int rv = 1; rv < nmatch; rv++) {
    if (-1 == pmatch [rv].rm_so) {
      skipped++;
      continue;
    }
    assert (-1 != pmatch [rv].rm_eo);
    while (skipped) {
      v.push_back (string(""));
      skipped--;
    }
    v.push_back (s.substr (pmatch [rv].rm_so, pmatch [rv].rm_eo-pmatch [rv].rm_so));
  }

  return true;
}

// Normalize UTF-8 encoding to detect
// http://www.faqs.org/rfcs/rfc2279.html section 8 attacks.

static string
normalizeutf8 (string &s)
{
  // Illegal sequences and their transformations are commented in the
  // code.

  string err = "";
  bool ok = true;

  // The string can only get shorter, so use it as result also.
  string::iterator d = s.begin ();
  for (string::const_iterator c = s.begin(); c != s.end(); ++c) {
    unsigned char u = *c;

    // Let US-ASCII, invalid bytes, and lonely continuation bytes pass.
    if (u < 0xc0 || u >= 0xfe) {
      *d++ = u;
      continue;
    }

    // 2 byte sequences
    if (s.end () < c+2
	|| (unsigned char)(*(c+1)) < 0x80
	|| (unsigned char)(*(c+1)) >= 0xc0) {
      // Cannot decode, pass as it is.
      *d++ = u;
      continue;
    }
    // 1100000x 10xxxxxx
    // -> 0xxxxxxx
    if (u <= 0xc1) {
      ok = false;
      ++c;
      // Decode to single byte US-ASCII
      *d++ = (0x3f & (unsigned char)(*c)) | ((u&1) << 6);
      continue;
    }
    if (u < 0xe0) {
      // Valid 2-byte sequence
      *d++ = u;
      *d++ = *++c;
      continue;
    }

    // 3 byte sequences
    if (s.end () < c+3
	|| (unsigned char)(*(c+2)) < 0x80
	|| (unsigned char)(*(c+2)) >= 0xc0) {
      // Cannot decode, pass as it is.
      *d++ = u;
      continue;
    }
    if (u == 0xe0) {
      // 11100000 1000000x 10xxxxxx
      // -> 0xxxxxxx
      if ((unsigned char)(*(c+1)) <= 0x81) {
	ok = false;
	u = *++c;
	++c;
	// Decode to single byte US-ASCII
	*d++ = (0x3f & (unsigned char)(*c)) | ((u&1) << 6);
	continue;
      }

      // 11100000 100xxxxx 10xxxxxx
      // -> 110xxxxx 10xxxxxx
      if ((unsigned char)(*(c+1)) <= 0x9f) {
	ok = false;
	*d++ = *++c | 0xc0;
	*d++ = *++c;
	continue;
      }
    }

    if (u < 0xf0) {
      // Valid 3-byte sequence
      *d++ = u;
      *d++ = *++c;
      *d++ = *++c;
      continue;
    }

    // 4 byte sequences
    if (s.end () < c+4
	|| (unsigned char)(*(c+3)) < 0x80
	|| (unsigned char)(*(c+3)) >= 0xc0) {
      // Cannot decode, pass as it is.
      *d++ = u;
      continue;
    }
    if (u == 0xf0) {
      if ((unsigned char)(*(c+1)) == 0x80) {
	// 11110000 10000000 1000000x 10xxxxxx
	// -> 0xxxxxxx
	if ((unsigned char)(*(c+2)) <= 0x81) {
	  ok = false;
	  c += 2;
	  u = *c++;
	  // Decode to single byte US-ASCII
	  *d++ = (0x3f & (unsigned char)(*c)) | ((u&1) << 6);
	  continue;
	}

	// 11110000 10000000 100xxxxx 10xxxxxx
	// -> 110xxxxx 10xxxxxx
	if ((unsigned char)(*(c+2)) <= 0x9f) {
	  ok = false;
	  c += 2;
	  *d++ = *c | 0xc0;
	  *d++ = *++c;
	  continue;
	}
      }

      // 11110000 1000xxxx 10xxxxxx 10xxxxxx
      // -> 1110xxxx 10xxxxxx 10xxxxxx
      if ((unsigned char)(*(c+1)) <= 0x8f) {
	  ok = false;
	  *d++ = *++c | 0xe0;
	  *d++ = *++c;
	  *d++ = *++c;
	  continue;
      }
    }

    if (u < 0xf8) {
      // Valid 4-byte sequence
      *d++ = u;
      *d++ = *++c;
      *d++ = *++c;
      *d++ = *++c;
      continue;
    }

    // 5 byte sequences
    if (s.end () < c+5
	|| (unsigned char)(*(c+4)) < 0x80
	|| (unsigned char)(*(c+4)) >= 0xc0) {
      // Cannot decode, pass as it is.
      *d++ = u;
      continue;
    }
    if (u == 0xf8) {
      if ((unsigned char)(*(c+1)) == 0x80) {
	if ((unsigned char)(*(c+2)) == 0x80) {
	  // 11111000 10000000 10000000 1000000x 10xxxxxx
	  // -> 0xxxxxxx
	  if ((unsigned char)(*(c+3)) <= 0x81) {
	    ok = false;
	    c += 3;
	    u = *c++;
	    // Decode to single byte US-ASCII
	    *d++ = (0x3f & (unsigned char)(*c)) | ((u&1) << 6);
	    continue;
	  }

	  // 11111000 10000000 10000000 100xxxxx 10xxxxxx
	  // -> 110xxxxx 10xxxxxx
	  if ((unsigned char)(*(c+3)) <= 0x9f) {
	    ok = false;
	    c += 3;
	    *d++ = *c | 0xc0;
	    *d++ = *++c;
	    continue;
	  }
	}

	// 11111000 10000000 1000xxxx 10xxxxxx 10xxxxxx
	// -> 1110xxxx 10xxxxxx 10xxxxxx
	if ((unsigned char)(*(c+2)) <= 0x8f) {
	  ok = false;
	  c += 2;
	  *d++ = *c | 0xe0;
	  *d++ = *++c;
	  *d++ = *++c;
	  continue;
	}
      }

      // 11111000 10000xxx 10xxxxxx 10xxxxxx 10xxxxxx
      // -> 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
      if ((unsigned char)(*(c+1)) <= 0x87) {
	ok = false;
	*d++ = *++c | 0xf0;
	*d++ = *++c;
	*d++ = *++c;
	*d++ = *++c;
	continue;
      }
    }

    if (u < 0xfc) {
      // Valid 5-byte sequence
      *d++ = u;
      *d++ = *++c;
      *d++ = *++c;
      *d++ = *++c;
      *d++ = *++c;
      continue;
    }

    // 6 byte sequences
    if (s.end () < c+6
	|| (unsigned char)(*(c+5)) < 0x80
	|| (unsigned char)(*(c+5)) >= 0xc0) {
      // Cannot decode, pass as it is.
      *d++ = u;
      continue;
    }
    if (u == 0xfc) {
      if ((unsigned char)(*(c+1)) == 0x80) {
	if ((unsigned char)(*(c+2)) == 0x80) {
	  if ((unsigned char)(*(c+3)) == 0x80) {
	    // 11111100 10000000 10000000 10000000 1000000x 10xxxxxx
	    // -> 0xxxxxxx
	    if ((unsigned char)(*(c+4)) <= 0x81) {
	      ok = false;
	      c += 4;
	      u = *c++;
	      // Decode to single byte US-ASCII
	      *d++ = (0x3f & (unsigned char)(*c)) | ((u&1) << 6);
	      continue;
	    }

	    // 11111100 10000000 10000000 10000000 100xxxxx 10xxxxxx
	    // -> 110xxxxx 10xxxxxx
	    if ((unsigned char)(*(c+4)) <= 0x9f) {
	      ok = false;
	      c += 4;
	      *d++ = *c | 0xc0;
	      *d++ = *++c;
	      continue;
	    }
	  }

	  // 11111100 10000000 10000000 1000xxxx 10xxxxxx 10xxxxxx
	  // -> 1110xxxx 10xxxxxx 10xxxxxx
	  if ((unsigned char)(*(c+3)) <= 0x8f) {
	    ok = false;
	    c += 3;
	    *d++ = *c | 0xe0;
	    *d++ = *++c;
	    *d++ = *++c;
	    continue;
	  }
	}

	// 11111100 10000000 10000xxx 10xxxxxx 10xxxxxx 10xxxxxx
	// -> 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
	if ((unsigned char)(*(c+2)) <= 0x87) {
	  ok = false;
	  c += 2;
	  *d++ = *c | 0xf0;
	  *d++ = *++c;
	  *d++ = *++c;
	  *d++ = *++c;
	  continue;
	}
      }

      // 11111100 100000xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
      // -> 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
      if ((unsigned char)(*(c+1)) <= 0x83) {
	ok = false;
	*d++ = *++c | 0xf8;
	*d++ = *++c;
	*d++ = *++c;
	*d++ = *++c;
	*d++ = *++c;
	continue;
      }
    }

    // Valid 6-byte sequence
    *d++ = u;
    *d++ = *++c;
    *d++ = *++c;
    *d++ = *++c;
    *d++ = *++c;
    *d++ = *++c;
  }

  // Result string may be shorter, so erase unused space.
  s.erase (d, s.end());

  if (!ok)
    err = "UTFATTACK45809jkHJSD82rk8903jdfj3";
  return err;
}

// Repeatedly try to decode character encoding.  Leave characters that
// cannot be decoded as they are.  Returns error message string.

static string
robust_iconv (iconv_t ic, string &message)
{
  string err;
  bool ok = true;

  size_t in_left = message.length();
  size_t out_left = in_left * 3 + 100;
  size_t out_chars = out_left;
  char *buf = new char [out_chars];
  assert (buf);
  char *icopy = new char [in_left];
  assert (icopy);

  char *obuf = buf;
  memcpy (icopy, message.c_str(), in_left);
  message.erase ();
  char *ibuf = icopy;

  while (in_left > 0) {
    if ((size_t)(-1) == iconv (ic, &ibuf, &in_left, &obuf, &out_left)) {
      // Also when errno != E2BIG, we first reserve more space if
      // output space is not available, so that we do not run out
      // later when copying the offending byte.
      if (E2BIG == errno || out_left == 0) {
	// This should be rare, we reserve so much buffer
	message += string (buf, out_chars - out_left);
	obuf = buf;
	out_left = out_chars;
      } else if (in_left > 0) {
	ok = false;
	*obuf++ = *ibuf++;
	out_left--;
	in_left--;
      }
    }
  }
  message += string (buf, out_chars - out_left);
  delete [] buf;
  delete [] icopy;

  if (!ok)
    err = "ICONVERROR5iorjkfewfmkdfs2lklkfsd";
  return err;
}

// According to BugTraq, user agents implement many different ways to
// handle invalid padding inside base64 data.  All of these are there:
//
// 1. threat padding character as end of the encoded data
// 2. ignore padding character
// 3. decode padding character as well as some other character from base64
//    alphabet
// 4. do something else ;-)
//
// As this software is not used in virus scanner, we just implement
// option 2 and add a marker for invalid padding.

// Modifies the argument, returns error message if any.

static string
base64decode (string &s)
{
  string result;
  int bits = 0;
  int buf = 0;
  int padding = 0;
  string errormessage;
  for (size_t pos = 0; pos < s.length (); ++pos) {
    char c = s[pos];
    int v;
    if (c >= 'A' && c <= 'Z')
      v = c - 'A';
    else if (c >= 'a' && c <= 'z')
      v = c - 'a' + 26;
    else if (c >= '0' && c <= '9')
      v = c - '0' + 52;
    else if ('+' == c)
      v = 62;
    else if ('/' == c)
      v = 63;
    else if ('=' == c) {
      padding++;
      continue;
    } else
      // unknown chars are ignored
      continue;
    if (padding) {
      errormessage = "X-warn: ksU7AwpcqQoiCC84ceueEqKn padding inside base64\n";
      padding = 0;
    }
    buf = buf << 6 | v;
    bits += 6;
    if (bits >= 8) {
      c = 255 & (buf >> (bits - 8));
      result += c;
      bits -= 8;
    }
  }
  if (!(0 == bits && 0 == padding
	|| 2 == bits && 1 == padding
	|| 4 == bits && 2 == padding))
    errormessage = "X-warn: jHnnb3URVED5UgX9fxnZfAsV invalid base64 padding\n"
      + errormessage;
  s = result;
  return errormessage;
}

static string
qpdecode (string &s, bool header)
{
  string result;
  for (size_t pos = 0; pos < s.length (); ++pos) {
    char c = s[pos];
    if (header && '_' == c)
      c = 0x20;
    else if ('=' == c
	     && pos+3 <= s.length ()
	     && isxdigit(s[pos+1])
	     && isxdigit(s[pos+2])) {
      c = (isdigit(s[pos+2]) ? s[pos+2] - '0' : toupper (s[pos+2]) - 'A' + 10)
	| 16 * (isdigit(s[pos+1]) ? s[pos+1] - '0' : toupper (s[pos+1]) - 'A' + 10);
      pos += 2;
    } else if ('=' == c
	       && pos+2 <= s.length ()
	       && ('\r' == s[pos+1] || '\n' == s[pos+1])) {
      if ('\r' == s[pos+1]) {
	if (pos + 3 <= s.length() && '\n' == s [pos+2])
	  ++pos;
	++pos;
      }
      if ('\n' == s[pos+1]) {
	if (pos + 3 <= s.length() && '\r' == s [pos+2])
	  ++pos;
	++pos;
      }
      continue;
    }
    result += c;
  }
  s = result;
  return ""; // never returns an error
}

static void
strtolower (string &s)
{
  for (string::iterator c = s.begin(); c != s.end(); ++c)
    *c = tolower(*c);
}

static void
strtoupper (string &s)
{
  for (string::iterator c = s.begin(); c != s.end(); ++c)
    *c = toupper(*c);
}



struct headerparams
{
  string value;
  map<string,string> options;
};

class esearcher {
private:
  struct entity
  {
    char *name;
    long code;
  };

  static const struct entity entities[];
  static map<string,long> emap;

public:
  esearcher ();
  // No need for destructor.
  long operator () (string e) {
    map<string,long>::const_iterator v = emap.find (e);
    if (emap.end() == v)
      return -1;
    return v->second;
  }
};

esearcher::esearcher ()
{
  if (emap.empty()) {
    for (const entity *eptr = entities;
	 eptr->name;
	 ++eptr) {
      emap [eptr->name] = eptr->code;
    }
  }
}

map<string,long> esearcher::emap;
const esearcher::entity esearcher::entities[] =
  {
    { "nbsp",   160 },
    { "iexcl",  161 },
    { "cent",   162 },
    { "pound",  163 },
    { "curren", 164 },
    { "yen",    165 },
    { "brvbar", 166 },
    { "sect",   167 },
    { "uml",    168 },
    { "copy",   169 },
    { "ordf",   170 },
    { "laquo",  171 },
    { "not",    172 },
    { "shy",    173 },
    { "reg",    174 },
    { "macr",   175 },
    { "deg",    176 },
    { "plusmn", 177 },
    { "sup2",   178 },
    { "sup3",   179 },
    { "acute",  180 },
    { "micro",  181 },
    { "para",   182 },
    { "middot", 183 },
    { "cedil",  184 },
    { "sup1",   185 },
    { "ordm",   186 },
    { "raquo",  187 },
    { "frac14", 188 },
    { "frac12", 189 },
    { "frac34", 190 },
    { "iquest", 191 },
    { "Agrave", 192 },
    { "Aacute", 193 },
    { "Acirc",  194 },
    { "Atilde", 195 },
    { "Auml",   196 },
    { "Aring",  197 },
    { "AElig",  198 },
    { "Ccedil", 199 },
    { "Egrave", 200 },
    { "Eacute", 201 },
    { "Ecirc",  202 },
    { "Euml",   203 },
    { "Igrave", 204 },
    { "Iacute", 205 },
    { "Icirc",  206 },
    { "Iuml",   207 },
    { "ETH",    208 },
    { "Ntilde", 209 },
    { "Ograve", 210 },
    { "Oacute", 211 },
    { "Ocirc",  212 },
    { "Otilde", 213 },
    { "Ouml",   214 },
    { "times",  215 },
    { "Oslash", 216 },
    { "Ugrave", 217 },
    { "Uacute", 218 },
    { "Ucirc",  219 },
    { "Uuml",   220 },
    { "Yacute", 221 },
    { "THORN",  222 },
    { "szlig",  223 },
    { "agrave", 224 },
    { "aacute", 225 },
    { "acirc",  226 },
    { "atilde", 227 },
    { "auml",   228 },
    { "aring",  229 },
    { "aelig",  230 },
    { "ccedil", 231 },
    { "egrave", 232 },
    { "eacute", 233 },
    { "ecirc",  234 },
    { "euml",   235 },
    { "igrave", 236 },
    { "iacute", 237 },
    { "icirc",  238 },
    { "iuml",   239 },
    { "eth",    240 },
    { "ntilde", 241 },
    { "ograve", 242 },
    { "oacute", 243 },
    { "ocirc",  244 },
    { "otilde", 245 },
    { "ouml",   246 },
    { "divide", 247 },
    { "oslash", 248 },
    { "ugrave", 249 },
    { "uacute", 250 },
    { "ucirc",  251 },
    { "uuml",   252 },
    { "yacute", 253 },
    { "thorn",  254 },
    { "yuml",   255 },
    { "quot",    34 },
    { "amp",     38 },
    { "lt",      60 },
    { "gt",      62 },
    { "apos", 39 },
    { "OElig",   338 },
    { "oelig",   339 },
    { "Scaron",  352 },
    { "scaron",  353 },
    { "Yuml",    376 },
    { "circ",    710 },
    { "tilde",   732 },
    { "ensp",    8194 },
    { "emsp",    8195 },
    { "thinsp",  8201 },
    { "zwnj",    8204 },
    { "zwj",     8205 },
    { "lrm",     8206 },
    { "rlm",     8207 },
    { "ndash",   8211 },
    { "mdash",   8212 },
    { "lsquo",   8216 },
    { "rsquo",   8217 },
    { "sbquo",   8218 },
    { "ldquo",   8220 },
    { "rdquo",   8221 },
    { "bdquo",   8222 },
    { "dagger",  8224 },
    { "Dagger",  8225 },
    { "permil",  8240 },
    { "lsaquo",  8249 },
    { "rsaquo",  8250 },
    { "euro",   8364 },
    { "fnof",     402 },
    { "Alpha",    913 },
    { "Beta",     914 },
    { "Gamma",    915 },
    { "Delta",    916 },
    { "Epsilon",  917 },
    { "Zeta",     918 },
    { "Eta",      919 },
    { "Theta",    920 },
    { "Iota",     921 },
    { "Kappa",    922 },
    { "Lambda",   923 },
    { "Mu",       924 },
    { "Nu",       925 },
    { "Xi",       926 },
    { "Omicron",  927 },
    { "Pi",       928 },
    { "Rho",      929 },
    { "Sigma",    931 },
    { "Tau",      932 },
    { "Upsilon",  933 },
    { "Phi",      934 },
    { "Chi",      935 },
    { "Psi",      936 },
    { "Omega",    937 },
    { "alpha",    945 },
    { "beta",     946 },
    { "gamma",    947 },
    { "delta",    948 },
    { "epsilon",  949 },
    { "zeta",     950 },
    { "eta",      951 },
    { "theta",    952 },
    { "iota",     953 },
    { "kappa",    954 },
    { "lambda",   955 },
    { "mu",       956 },
    { "nu",       957 },
    { "xi",       958 },
    { "omicron",  959 },
    { "pi",       960 },
    { "rho",      961 },
    { "sigmaf",   962 },
    { "sigma",    963 },
    { "tau",      964 },
    { "upsilon",  965 },
    { "phi",      966 },
    { "chi",      967 },
    { "psi",      968 },
    { "omega",    969 },
    { "thetasym", 977 },
    { "upsih",    978 },
    { "piv",      982 },
    { "bull",     8226 },
    { "hellip",   8230 },
    { "prime",    8242 },
    { "Prime",    8243 },
    { "oline",    8254 },
    { "frasl",    8260 },
    { "weierp",   8472 },
    { "image",    8465 },
    { "real",     8476 },
    { "trade",    8482 },
    { "alefsym",  8501 },
    { "larr",     8592 },
    { "uarr",     8593 },
    { "rarr",     8594 },
    { "darr",     8595 },
    { "harr",     8596 },
    { "crarr",    8629 },
    { "lArr",     8656 },
    { "uArr",     8657 },
    { "rArr",     8658 },
    { "dArr",     8659 },
    { "hArr",     8660 },
    { "forall",   8704 },
    { "part",     8706 },
    { "exist",    8707 },
    { "empty",    8709 },
    { "nabla",    8711 },
    { "isin",     8712 },
    { "notin",    8713 },
    { "ni",       8715 },
    { "prod",     8719 },
    { "sum",      8721 },
    { "minus",    8722 },
    { "lowast",   8727 },
    { "radic",    8730 },
    { "prop",     8733 },
    { "infin",    8734 },
    { "ang",      8736 },
    { "and",      8743 },
    { "or",       8744 },
    { "cap",      8745 },
    { "cup",      8746 },
    { "int",      8747 },
    { "there4",   8756 },
    { "sim",      8764 },
    { "cong",     8773 },
    { "asymp",    8776 },
    { "ne",       8800 },
    { "equiv",    8801 },
    { "le",       8804 },
    { "ge",       8805 },
    { "sub",      8834 },
    { "sup",      8835 },
    { "nsub",     8836 },
    { "sube",     8838 },
    { "supe",     8839 },
    { "oplus",    8853 },
    { "otimes",   8855 },
    { "perp",     8869 },
    { "sdot",     8901 },
    { "lceil",    8968 },
    { "rceil",    8969 },
    { "lfloor",   8970 },
    { "rfloor",   8971 },
    { "lang",     9001 },
    { "rang",     9002 },
    { "loz",      9674 },
    { "spades",   9824 },
    { "clubs",    9827 },
    { "hearts",   9829 },
    { "diams",    9830 },
    { 0, -1 }
  };

string
utf8code (long ucs4)
{
  int i;
  char utf8[7];

  if (ucs4 <= 0x7fl) {
    utf8[0] = ucs4;
    i = 1;
  } else if (ucs4 <= 0x7ffl) {
    utf8[0] = 0xc0 | (ucs4 >> 6);
    utf8[1] = 0x80 | (ucs4 & 0x3f);
    i = 2;
  } else if (ucs4 <= 0xffffl) {
    utf8[0] = 0xe0 | (ucs4 >> 12);
    utf8[1] = 0x80 | ((ucs4 >> 6) & 0x3f);
    utf8[2] = 0x80 | (ucs4 & 0x3f);
    i = 3;
  } else if (ucs4 <= 0x1fffffl) {
    utf8[0] = 0xf0 | (ucs4 >> 18);
    utf8[1] = 0x80 | ((ucs4 >> 12) & 0x3f);
    utf8[2] = 0x80 | ((ucs4 >> 6) & 0x3f);
    utf8[3] = 0x80 | (ucs4 & 0x3f);
    i = 4;
  } else if (ucs4 <= 0x3ffffffl) {
    utf8[0] = 0xf8 | (ucs4 >> 24);
    utf8[1] = 0x80 | ((ucs4 >> 18) & 0x3f);
    utf8[2] = 0x80 | ((ucs4 >> 12) & 0x3f);
    utf8[3] = 0x80 | ((ucs4 >> 6) & 0x3f);
    utf8[4] = 0x80 | (ucs4 & 0x3f);
    i = 5;
  } else if (ucs4 <= 0x7fffffffl) {
    utf8[0] = 0xfc | (ucs4 >> 30);
    utf8[1] = 0x80 | ((ucs4 >> 24) & 0x3f);
    utf8[2] = 0x80 | ((ucs4 >> 18) & 0x3f);
    utf8[3] = 0x80 | ((ucs4 >> 12) & 0x3f);
    utf8[4] = 0x80 | ((ucs4 >> 6) & 0x3f);
    utf8[5] = 0x80 | (ucs4 & 0x3f);
    i = 6;
  } else
    return "&invalid;";

  utf8[i] = '\0';
  return string (utf8);
}

string
decode_url (string s)
{
  static const regex urlenc ("%[0-9A-Fa-f][0-9A-Fa-f]");
  vector<string> v;
  string ret;
  while (urlenc.exec (s, v)) {
    ret += v[0];
    char c = strtol (v[1].c_str() + 1, 0, 16);
    if (c > 32 && c < 127 && c != ';' && c != '/' && c != '?' && c != ':'
	&& c != '@' && c != '=' && c != '&' && c != '#' && c != '+'
	&& c != '%')
      ret += c;
    else
      ret += v[1];
    s = v[2];
    v.clear ();
  }
  return ret + s;
}

void
remove_html (string &message)
{
  string m1;
  vector<string> v;

  // HTML entities are expanded to UTF-8 encoded characters.
  // But &, <, >, " and ' are left as entities, to be decoded later.
  // Maybe they should be left encoded?
  static const regex ent ("&(#([0-9]+)|#x([0-9a-f]+)|[0-9a-z]+);?");
  while (ent.exec (message, v)) {
    m1 += v[0];
    // If the entry is valid, let's replace it.

    long code;
    if (v.size() >= 5 && v[4].length())
      // Decimal entity mathed.
      code = strtol (v[4].c_str(), 0, 10);
    else if (v.size() >= 6 && v[5].length())
      // Hexadecimal entry matched.
      code = strtol (v[4].c_str(), 0, 16);
    else {
      // Entity name, maybe.
      esearcher finde;
      code = finde (v [3]);
    }

    if (code >= 0 && code != '<' && code != '>' && code != '&'
	&& code != '\"' && code != '\'') {
      // Turn &nbsp; into regular space.
      if (160 == code)
	code = ' ';
      m1 += utf8code (code);
    } else
      // Make sure the entity we leave there has semicolon at the end.
      m1 += "&" + v[3] + ";";
    message = v[2];
    v.clear ();
  }
  message = m1 + message;
  m1 = "";

  // HTML comments ( a.k.a. hypertextus interruptus) are removed.
  static const regex htmlcomm ("<!--([^-]|-[^-]|--[^>])*-->");
  while (htmlcomm.exec (message, v)) {
    // Prematch
    m1 += v[0];
    // and postmatch.  Omit match.
    message = v[2];
    v.clear ();
  }
  message = m1 + message;
  m1 = "";

  // To prevent <a> tag exception below to be misused to split words,
  // remove <a> tags which do not contain anything.
  static const regex fakea ("<a([[:blank:]][^>]*)?><[[:blank:]]*/[[:blank:]]*a[[:blank:]]*>", REG_EXTENDED | REG_ICASE);
  while (fakea.exec (message, v)) {
    m1 += v[0];
    message = v[2];
    v.clear ();
  }
  message = m1 + message;
  m1 = "";

  // Certain tags are expanded to spaces.
  static const regex tospace ("</?(table|br|tr|td|hr|h[1-6]|dl|dd|dt|ul|li|ol|p|object|div)([[:blank:]][^>]*)?/?>",
			      REG_EXTENDED | REG_ICASE);
  while (tospace.exec (message, v)) {
    m1 += v[0] + " ";
    message = v[2];
    v.clear ();
  }
  message = m1 + message;
  m1 = "";

  // All other tags are expanded to nothing.
  static const regex tag ("<[^>]*>");
  while (tag.exec (message, v)) {
    m1 += v[0];
    // Some tags, like <a> and <img> are preserved still.
    static const regex atag ("<(a|/[[:blank:]]*a|img)[^a-z0-9]",
			     REG_EXTENDED | REG_ICASE);
    if (atag.exec (v[1]))
      m1 += v[1];
    message = v[2];
    v.clear ();
  }
  message = m1 + message;
  m1 = "";

  // href="" and src="" tags are expanded for %xx encoding
  static const regex href ("([ \t\n\r](href|src)=\"?)([^ \t\n\r\">]+)(\"?)",
			   REG_EXTENDED | REG_ICASE);
  while (href.exec (message, v)) {
    m1 += v[0] + v[3] + decode_url(v[5]) + v[6];
    message = v[2];
    v.clear ();
  }
  message = m1 + message;
}


void
normalize (string &message, string content_type = "text/plain")
{

  // Content-* hearers are decoded to this structure:
  map<string,headerparams> contentheaders;

  /*

1)
Parse a message into headers and body
decode headers to utf-8
also decode another set of headers to structured form

multipart:
	Parse into separate message bodies
	recursively call 1) for these bodies

rfc822:
	call 1) for this body

*:
	decode encoding and translate charset

  */

  static const regex splithead ("\n\r?|\r\n?");
  static regex cpreg ("^CP-([0-9][0-9][0-9][0-9]?)$", REG_EXTENDED | REG_ICASE);
  vector<string> headers;
  while (message.length() > 0) {
    vector<string> v;
    string h;
    if (splithead.exec (message, v) && 3 == v.size()) {
      h = v[0]; // prematch match
      message = v[2]; // postmatch
    } else {
      h = message;
      message = "";
    }

    // Empty line ends header section.
    if (0 == h.length ())
      break;

    // Trim the trailing spaces
    size_t spaces = 0;
    while (h.length () > spaces && isspace (h [h.length()-1-spaces]))
      ++spaces;
    if (spaces)
      h.erase (h.length()-spaces);

    if (0 == h.length ()) {
      // Continuation line consisting of just spaces is easy to append
      // to previous header.  Let's see.. start.. done.
      //cerr << "Empty continuation" << endl;
      continue;
    }

    if (headers.size() > 0 && isspace (h[0])) {
      // continuation header, append to previous header
      spaces = 1;
      while (isspace (h[spaces]))
	++spaces;
      headers.back() += " " + string (h, spaces);
      //cerr << "Catenated: " << h << endl;
      continue;
    }

    // Just the usual header.
    //cerr << "found header " << h << endl;
    headers.push_back (h);
  }

  // Now we have headers in vector<string> headers and message body in
  // string message.

  // Go through the headers, decode them to utf-8 and parse
  // information out.

  for (vector<string>::iterator h = headers.begin(); h != headers.end(); ++h) {
    size_t pos;
    for (pos = 0; pos < h->length () && (*h)[pos] != ':'; ++pos)
      ;
    if (0 == pos || pos >= h->length ())
      // Not really a header.
      continue;

    string header_name = h->substr (0, pos);
    strtolower (header_name);

    ++pos;
    while (pos < h->length () && isspace ((*h)[pos]))
      ++pos;

    // The working buffer.
    string b = h->substr (pos);
    string value;

    // Erase the rest of the header, to be replaced by decoded string.
    h->erase (pos);

    static const regex enchead ("=\\?([-!#$%&\'*+0-9A-Z^_`a-z{|}~]+)\\?(B|Q|b|q)\\?([^ \t\\?]+)\\?=");

    bool prevenc = false;
    while (b.length () > 0) {
      vector<string> v;
      if (!enchead.exec (b, v) || 6 != v.size()) {
	// cerr << progname << ": *** " << b << " ***" << endl;
	value += b;
	// b = "";
	break;
      }

      if (prevenc) {
	// Whitespace must be removed from between encoded atoms.
	size_t pos = 0;
	do {
	  if (pos >= v[0].length ()) {
	    v[0] = "";
	    break;
	  }
	} while (isspace (v[0][pos++]));
      }
      prevenc = true;

      value += v[0]; // prematch
      string token = v[1]; // full match
      b = v[2]; // postmatch
      // cerr << progname << ": +++ " << token << " +++" << endl;
      string in_charset = v[3];
      strtoupper (in_charset);
      if (in_charset.empty ()
	  || "US-ASCII" == in_charset
	  || "ASCII-US" == in_charset
	  || "DEFAULT" == in_charset)
	in_charset = "ISO-8859-1";
      else if ("UNICODE-1-1-UTF-7" == in_charset)
	in_charset = "UTF-7";
      else if ("UNICODE-1-1-UTF-8" == in_charset)
	in_charset = "UTF-8";
      else if ("CHINESEBIG5" == in_charset)
	in_charset = "BIG5";
      else if ("KS_C_5601-1987" == in_charset)
	in_charset = "CP949";
      else if ("GB2312_CHARSET" == in_charset)
	in_charset = "GB2312";
      else {
	vector<string> ch;
	if (cpreg.exec (in_charset, ch)) {
	  in_charset = "CP" + ch[3];
	}
      }
      bool do_conversion = (in_charset != "UTF-8"
			    && in_charset != "UTF8");
      iconv_t ic = (iconv_t)(-1);
      if (do_conversion) {
	ic = iconv_open ("UTF-8", in_charset.c_str());
	if ((iconv_t)(-1) == ic) {
	  //cerr << progname << ": \"" << token << "\": unknown character set in encoded header." << endl;
	  value += " " + v[3] + " BADHEADERCHARSETckW2eAWEEyAGmHQK ";
	  do_conversion = false;
	}
      }
      string decodingerror;
      if ("B" == v[4] || "b" == v[4])
	decodingerror = base64decode (v[5]);
      else
	decodingerror = qpdecode (v[5], true);
      value += decodingerror;

      // See if it was unnecessary to encode this text.  That is a
      // sign of spam.

      bool onlyascii = true;
      // These popular charsets have US-ASCII 7-bit subset.
      // Others have not, not popular ones at least.
      if ("UTF-8" != in_charset
	  && "UTF8" != in_charset
	  && "GB2312" != in_charset
	  && strncmp (in_charset.c_str(), "ISO-8859-", 9))
	onlyascii = false;
      else
	for (unsigned int i = 0; onlyascii && i < v[5].length(); ++i)
	  onlyascii = isascii(v[5][i]) && isprint(v[5][i]) && '=' != v[5][i];

      if (do_conversion) {
	string err = robust_iconv (ic, v[5]);
	iconv_close (ic);
	if (err.empty ())
	  token = v[5];
	else
	  // If the conversion failed, preserve as much information as
	  // possible about what failed.
	  token = "=?" + v[3] + "?" + v[4] + "?" + err + " " + v[5] + "?=";
      } else {
	string err = normalizeutf8 (v[5]);
	if (!err.empty())
	  // If the conversion failed, preserve as much information as
	  // possible about what failed.
	  token = "=?" + v[3] + "?" + v[4] + "?" + err + " " + v[5] + "?=";
	else
	  token = v[5];
      }

      if (onlyascii)
	// Mark the suspected spam header
	value += " ONLYASCIIKFrjuZnFvmJJdrRkeXrd95wu ";

      value += token;
    }

    // Re-asseblme the header line.
    *h += value;

    // Now we have header_name and value.  Do additional parsing
    // if the name starts with 'content-'.

    if (!strncmp (header_name.c_str(), "content-", 8)) {
      vector<string> v;
      static const regex semicol ("[ \t]*;[ \t]*");
      if (semicol.exec (value, v)) {
	strtolower (v[0]);
	contentheaders[header_name].value = v[0];
	value = v[2];
	static const regex assignment ("[\t ]*=[\t ]*(\"([^\"]+)?\"|[^;]*[^ \t;])?[ \t]*(;[ \t]*)?");
	v.clear ();
	while (assignment.exec (value, v)) {
	  strtolower (v[0]);
	  if (v.size() >= 5 && (v[4].length() > 0 || v[3] == "\"\""))
	    contentheaders[header_name].options[v[0]] = v[4];
	  else if (v.size() < 4 || v[3] == "\"\"")
	    contentheaders[header_name].options[v[0]] = "";
	  else
	    contentheaders[header_name].options[v[0]] = v[3];
	  value = v[2];
	  v.clear ();
	}
      } else
	contentheaders[header_name].value = value;
    }
  }

#if 0
  // Debug output of headers to cerr
  for (map<string,headerparams>::const_iterator ch = contentheaders.begin();
       ch != contentheaders.end(); ++ch) {
    cerr << ch->first << ": " << ch->second.value;
    for (map<string,string>::const_iterator op = ch->second.options.begin();
	 op != ch->second.options.end(); ++op) {
      cerr << "; " << op->first << "=\"" << op->second << "\"";
    }
    cerr << endl;
  }
#endif

  // Decide what to do.

  string decodingerror;
  if (contentheaders["content-transfer-encoding"].value == "quoted-printable") {
    decodingerror = qpdecode (message, false);
  } else if (contentheaders["content-transfer-encoding"].value == "base64") {
    decodingerror = base64decode (message);
  }

  if (contentheaders["content-type"].value.length())
    content_type = contentheaders["content-type"].value;

  // cerr << progname << ": **Type: " << content_type << endl;

  if (!strncmp (content_type.c_str(), "multipart/", 10)) {
    string default_content = "text/plain";
    if (content_type == "multipart/digest")
      default_content = "message/rfc822";

    string boundarystr = contentheaders["content-type"].options["boundary"];

    // If the boundary is not specified, leave message intact.
    if (boundarystr.length()) {
      string newmessage;
      regex boundary (string("(\r\n?|\n\r?)--") + regex::escape (boundarystr) + "(--)?[ \t]*(\r\n?|\n\r?|$)");
      vector<string> v;
      bool first = true;
      bool seen_end = false;
      message = "\n" + message;
      while (boundary.exec (message, v)) {
	bool prematchempty = false;
	if (first) {
	  // prematch is the mime redundant comment part before first
	  // message part.
	  // Remove the newline added above.
	  if (v[0].length() > 0)
	    v[0].erase (0, 1);
	  else
	    prematchempty = true;
	  first = false;
	} else {
	  // prematch is a message.
	  normalize (v[0], default_content);
	  newmessage += "\n";
	}
	if (!prematchempty)
	  newmessage += v[0] + "\n";
	newmessage += "---"; // "--" + boundarystr;
	message = v[2];
	if ("--" == v[4]) {
	  seen_end = true;
	  break;
	}
	v.clear();
      }
      if (first && message.length () > 0)
	// Remove the added newline in the beginning
	message.erase (0, 1);
      // The part after the mime end limiter is added as it is, except
      // possibly truncated message is decoded anyway.
      if (!seen_end && message.length () > 0) {
	//cerr << "Warning: ending mime delimiter " << boundarystr << " not seen" << endl;
	normalize (message, default_content);
      }
      if (!first)
	message = newmessage + "--\n" + message;
    }
  } else if (!strncmp (content_type.c_str(), "text/", 5)) {
    string in_charset = contentheaders["content-type"].options["charset"];
    strtoupper (in_charset);
    if (in_charset.empty()
	|| "US-ASCII" == in_charset
	|| "ASCII-US" == in_charset
	|| "DEFAULT" == in_charset)
      in_charset = "ISO-8859-1";
    else if ("UNICODE-1-1-UTF-7" == in_charset)
      in_charset = "UTF-7";
    else if ("UNICODE-1-1-UTF-8" == in_charset)
      in_charset = "UTF-8";
    else if ("CHINESEBIG5" == in_charset)
      in_charset = "BIG5";
    else if ("KS_C_5601-1987" == in_charset)
      in_charset = "CP949";
    else if ("GB2312_CHARSET" == in_charset)
      in_charset = "GB2312";
    else {
      vector<string> ch;
      if (cpreg.exec (in_charset, ch)) {
	in_charset = "CP" + ch[3];
      }
    }

    if (in_charset != "UTF-8" && in_charset != "UTF8") {
      iconv_t ic;
      ic = iconv_open ("UTF-8", in_charset.c_str());
      if ((iconv_t)(-1) == ic) {
	//cerr << progname << ": \"" << in_charset << "\": unknown character set in message body." << endl;
	headers.push_back ("X-warn: 3j94twCXM5njkztE bad charset in body");
      } else {
	string err = robust_iconv (ic, message);
	iconv_close (ic);
	if (!err.empty())
	  message += " " + err;
      }
    } else {
      string err = normalizeutf8 (message);
      if (!err.empty())
	message += " " + err;
    }
  } else if (content_type == "message/rfc822") {
    normalize (message);
  } else if (message.length() > 2*1024) {
    message.erase (2*1024);
  }
  message += decodingerror;

  // Reassemble headers to the message.  Filter away the headers that
  // have been added by possible other filters.  Some headers mail
  // user agents like to add are also stripped.  You should add to this
  // list anything added to the messages locally.  The list is case
  // insensitive and terminated by a null pointer.

  const char * const purgedheaders[10] =
    {
      "X-Spam-",           // Added by SpamAssasin for example
      "X-CRM114-",         // Added by CRM114
      "X-Virus-",          // Added by ClamAV
      "X-UID:",            // added by Pine mail user agent
      "Status:",           // added by Pine mail user agent
      "X-Status:",         // added by Pine mail user agent
      "X-Keywords:",       // added by Pine mail user agent
      0
    };

  string hh;
  for (vector<string>::iterator h = headers.begin(); h != headers.end(); ++h) {
    const char * const *ph = purgedheaders;
    while (*ph && strncasecmp (h->c_str(), *ph, strlen(*ph)))
      ++ph;

    // If the header is in the purge list, discard it
    if (*ph)
      continue;
    hh += *h + "\n";
  }

  if (content_type != "message/rfc822"
      && strncmp (content_type.c_str(), "multipart/", 10)) {
    string origm = message;
    remove_html(message);
    if (origm != message)
      message = origm + "\n------\n" + message;
  }

  message = hh + "\n" + message;
}

void
normalize_filter (istream &inf, ostream &of)
{
  const int bufsize = 65536;
  char *buf = new char [bufsize];
  assert (buf);

  string message;
  for (;;) {
    inf.read (buf, bufsize);
    if (0 == inf.gcount()) {
      if (inf.eof())
	break;
      //cerr << progname << ": Read error" << endl;
      exit (1);
    }
    message.append (buf, inf.gcount());

    // To prevent DoS attacks against this program or filters, cut the
    // message size at hard limit.  If you need to make this
    // configurable, feel free to do it..
    if (message.length() >= 1024*1024)
      break;
  }
  delete [] buf;

  normalize (message);

  // Remove any nulls in output.
  for (;;) {
    string::size_type np;
    np = message.find_first_of ('\0');
    if (string::npos == np) {
      of << message;
      break;
    }
    if (0 != np)
      of << message.substr (0, np);
    np = message.find_first_not_of ('\0', np);
    if (string::npos == np)
      break;
    message.erase (0, np);
  }
}
/*
static void
usage (const char *prog) {
  cerr << "Normalizemime Copyright © 2003-2004 Jaakko Hyvätti" << endl
       << "$Id: normalizemime.cc,v 1.19 2008/05/19 09:13:18 jaakko Exp jaakko $" << endl
       << "Usage: " << prog << " infile outfile" << endl
       << "or " << prog << " < infile > outfile" << endl
       << "\n\
This is a mime email message parser to be used as a preprocessor\n\
for email classification software.\n\
\n\
Tries to normalize the content to 8bit encoding with utf-8 character set.\n\
Appends a copy of message body with HTML removed (IMG and A unaffected).\n\
Decodes HTML entities, like &auml; or &#228;\n\
Limits the size of attached binary files and removes null characters.\n\
Decodes URL encoding in HREF and SRC parameters in html.\n\
\n\
These are the headers that are removed from the messages:\n\
X-Spam-*\n\
X-CRM114-Status:\n\
X-CRM114-Version:\n\
Status: \n\
X-Status:\n\
X-Keywords:\n\
\n\
These text strings are inserted if base64 decoding is detected to contain\n\
possible attacks against decoders and virus/spam scanners:\n\
\n\
X-warn: jHnnb3URVED5UgX9fxnZfAsV invalid base64 padding\n\
X-warn: ksU7AwpcqQoiCC84ceueEqKn padding inside base64\n\
\n\
If the base64 string was inside a header, the headers get mangled \n\
totally, so this is not strictly speaking a header but just a word token\n\
that text classifier could learn.\n\
\n\
String ONLYASCIIKFrjuZnFvmJJdrRkeXrd95wu is added if the header was\n\
encoded even if it only consisted of US-ASCII.\n\
String BADHEADERCHARSETckW2eAWEEyAGmHQK is added if the encoding in header\n\
was not recognized.\n\
\n\
This header is added if the message body charset is not recognized:\n\
\n\
X-warn: 3j94twCXM5njkztE bad charset in body\n\
\n\
String ICONVERROR5iorjkfewfmkdfs2lklkfsd is added when the first\n\
charset coversion error is detected.\n\
String UTFATTACK45809jkHJSD82rk8903jdfj3 is added if suspicious\n\
UTF-8 encodings are used.\n\
";
  exit (EX_USAGE);
  // Todo:
  // Converts unicode to Normalization Form KD (NFKD) and strips marks.
}

int
main (int argc, char **argv)
{
  if ((progname = strrchr (argv [0], '/')))
    progname++;
  else
    progname = argv [0];
  if (argc > 3
      || (argc >= 2
	  && (!strcmp ("--help", argv[1]) || !strcmp ("-h", argv[1]))))
    usage(argv[0]);

  if (argc >= 2) {
    ifstream inf (argv[1]);
    if (!inf)
      usage (argv[0]);
    if (argc >= 3) {
      ofstream of (argv[2]);
      normalize_filter (inf, of);
    } else
      normalize_filter (inf, cout);
  } else
    normalize_filter (cin, cout);

  return 0;
}
*/
