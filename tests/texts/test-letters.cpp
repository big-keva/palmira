# include "texts/DOM-text.hpp"
# include "texts/DOM-dump.hpp"
# include "texts/DOM-load.hpp"
# include <moonycode/codes.h>
# include <mtc/arena.hpp>
# include <mtc/json.h>

int  main()
{
  auto  memo = mtc::Arena();
  auto  text = memo.Create<palmira::texts::BaseDocument<mtc::Arena::allocator<char>>>();
  const char  json[] =
    "[\n"
    "  \"aaa\",\n"
    "  \"bbb\",\n"
    "  { \"body\": [\n"
    "    \"first line in <body>\",\n"
    "    \"second line in <body>\",\n"
    "    { \"p\": [\n"
    "      \"first line in <p> tag\",\n"
    "      \"second line in <p> tag\"\n"
    "    ] },\n"
    "    \"third line in <body>\",\n"
    "    { \"span\": \"text line in <span>\" },\n"
    "    \"fourth line in <body>\"\n"
    "  ] },\n"
    "  \"last text line \\\"with quotes\\\"\""
    "]\n";

  text->AddString( 0U, "aaa" );
  text->AddString( codepages::mbcstowide( codepages::codepage_utf8, "bbb" ) );

  {
    auto  body = text->AddTextTag( "body" );

      body->AddString( codepages::codepage_utf8, "first line in <body>" );
      body->AddString( codepages::mbcstowide( codepages::codepage_utf8, "second line in <body>" ) );

      auto  para = body->AddTextTag( "p" );

        para->AddString( codepages::codepage_utf8, "first line in <p> tag" );
        para->AddString( codepages::codepage_utf8, "second line in <p> tag" );

      body->AddString( codepages::codepage_utf8, "third line in <body>" );

      auto  span = body->AddTextTag( "span" );
        span->AddString( codepages::codepage_utf8, "text line in <span>" );
//      para->AddString( codepages::codepage_utf8, "must cause exception" );

      body->AddString( codepages::codepage_utf8, "fourth line in <body>" );

    text->AddString( codepages::codepage_utf8, "last text line \"with quotes\"" );
  }

  text->Serialize( palmira::texts::dump_as::Json( palmira::texts::dump_as::MakeSink( stdout ) ).ptr() );
    fputc( '\n', stdout );
  text->Serialize( palmira::texts::dump_as::Tags( palmira::texts::dump_as::MakeSink( stdout ) ).ptr() );

  text->clear();

  palmira::texts::load_as::Json( text, palmira::texts::load_as::MakeSource( json ) );

  text->Serialize( palmira::texts::dump_as::Tags( palmira::texts::dump_as::MakeSink( stdout ) ).ptr() );
}
