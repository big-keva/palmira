# include "../../tinyxml/load-xml-text.hpp"
# include <mtc/wcsstr.h>
# include <tinyxml2.h>
# include <map>

namespace palmira {
namespace tinyxml {

  class Parser
  {
    unsigned                        encode = codepages::codepage_utf8;
    std::map<std::string, TagMode>  tagMap;

  public:
    Parser( const TagModes& modes ):
      tagMap( modes ) {}

  public:
    void  Decl( DelphiX::textAPI::IText* text, const char* decl );
    void  Elem( DelphiX::textAPI::IText* text, const tinyxml2::XMLElement& );
    void  Load( DelphiX::textAPI::IText* text, const tinyxml2::XMLNode& );

  protected:
    auto  Decl( const char* ) const -> std::map<std::string, std::string>;
    auto  Mode( const char* ) const -> TagMode;

  };

  void  Parser::Decl( DelphiX::textAPI::IText*, const char* dcl )
  {
    auto  decmap = Decl( dcl );
    auto  encode = decmap.find( "encoding" );

    if ( encode != decmap.end() )
    {
      if ( mtc::w_strcasecmp( encode->second.c_str(), "utf-8" ) == 0 )    this->encode = codepages::codepage_utf8;
        else
      if ( mtc::w_strcasecmp( encode->second.c_str(), "windows" ) == 0 )  this->encode = codepages::codepage_1251;
        else
      if ( mtc::w_strcasecmp( encode->second.c_str(), "iso-8859" ) == 0 ) this->encode = codepages::codepage_iso;
        else
      throw std::invalid_argument( mtc::strprintf( "invalid encoding '%s'", encode->second.c_str() ) );
    }
  }

  void  Parser::Elem( DelphiX::textAPI::IText* text, const tinyxml2::XMLElement& elem )
  {
    if ( elem.Value() != nullptr )
    {
      for ( auto attr = elem.FirstAttribute(); attr != nullptr; attr = attr->Next() )
        if ( attr->Value() != nullptr )
          text->AddMarkup( attr->Name() )->AddString( attr->Value() );

      for ( auto next = elem.FirstChild(); next != nullptr; next = next->NextSibling() )
        Load( text, *next );
    }
  }

  void  Parser::Load( DelphiX::textAPI::IText* text, const tinyxml2::XMLNode& node )
  {
    if ( node.ToDeclaration() != nullptr )
      return Decl( text, node.ToDeclaration()->Value() );

    if ( node.ToElement() != nullptr )
    {
      switch ( Mode( node.ToElement()->Value() ) )
      {
        case TagMode::remove:
          return (void)NULL;
        case TagMode::ignore:
          return Elem( text, *node.ToElement() );
        default:
          return Elem( text->AddMarkup( node.ToElement()->Value() ), *node.ToElement() );
      }
    }

    if ( node.ToText() != nullptr && node.ToText()->Value() != nullptr )
      text->AddString( node.ToText()->Value() );
  }

  auto  Parser::Decl( const char* decl ) const -> std::map<std::string, std::string>
  {
    auto  outmap = std::map<std::string, std::string>();

    for ( decl = mtc::ltrim( decl ); *decl != '\0'; decl = mtc::ltrim( decl ) )
    {
      auto  keytop = decl;
      auto  keyend = decl;

      while ( *keyend != '\0' && *keyend != '=' && !mtc::isspace( *keyend ) )
        ++keyend;

      if ( *keyend == '=' )
      {
        auto  valtop = mtc::ltrim( keyend + 1 );
        auto  valend = valtop;

        while ( *valend != '\0' && !mtc::isspace( *valend ) )
          ++valend, ++decl;

        if ( *valtop == '"' || *valtop == '\'' )
          ++valtop;
        if ( valend > valtop && (valend[-1] == '"' || valend[-1] == '\'') )
          --valend;

        outmap.insert( { { keytop, keyend }, { valtop, valend } } );
      } else outmap.insert( { { keytop, decl = keyend }, {} } );
    }
    return outmap;
  }

  auto  Parser::Mode( const char* mkup ) const -> TagMode
  {
    auto  pmap = tagMap.find( mkup );

    return pmap != tagMap.end() ? pmap->second : TagMode::undefined;
  }

  void  LoadObject( DelphiX::textAPI::IText* text, const TagModes& maps, const char* buff, size_t size )
  {
    Parser                load( maps );
    tinyxml2::XMLDocument xdoc;

    if ( xdoc.Parse( (const char*)buff, size ) != tinyxml2::XML_SUCCESS )
      throw std::runtime_error( mtc::strprintf( "failed to parse XML, error '%s:%s'",
        xdoc.ErrorName(), xdoc.ErrorStr() ) );

    for ( auto child = xdoc.FirstChild(); child != nullptr; child = child->NextSibling() )
      load.Load( text, *child );
  }

  void  LoadObject( DelphiX::textAPI::IText* text, const TagModes& maps, FILE* file )
  {
    Parser                load( maps );
    tinyxml2::XMLDocument xdoc;

    if ( xdoc.LoadFile( file ) != tinyxml2::XML_SUCCESS )
      throw std::runtime_error( mtc::strprintf( "failed to parse XML, error '%s:%s'",
        xdoc.ErrorName(), xdoc.ErrorStr() ) );

    for ( auto child = xdoc.FirstChild(); child != nullptr; child = child->NextSibling() )
      load.Load( text, *child );
  }

}}
