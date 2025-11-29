# include <mtc/iStream.h>
# include <unistd.h>
#include <remottp/request.hpp>
# include <sys/socket.h>

class ChunkedStream: public mtc::IByteStream
{
  int   sockFd;
  char  buffer[0x400];
  char* bufptr = buffer;

  std::atomic_long  refcount = 0;

public:
  uint32_t  Get(       void*, uint32_t ) override;
  uint32_t  Put( const void*, uint32_t ) override;

protected:
  int       put( const void*, uint32_t );

};

uint32_t  ChunkedStream::Get( void* pv, uint32_t cc )
{
  auto  ptrbuf = static_cast<char*>( pv );
  auto  ptrend = static_cast<char*>( pv ) + cc;

// flush the rest of data if present
  if ( bufptr != buffer )
  {
    char  header[0x20];
    int   hdrlen = sprintf( header, "%X\r\n", unsigned(bufptr - buffer) );

    if ( put( header, hdrlen ) < 0 )
    {
      close( sockFd );
      return uint32_t(-1);
    }
    if ( put( buffer, bufptr - buffer ) < 0 )
    {
      close( sockFd );
      return uint32_t(-1);
    }
    if ( put( "\r\n\r\n", 4 ) < 0 )
    {
      close( sockFd );
      return uint32_t(-1);
    }
    bufptr = buffer;
  }

// begin read data
  while ( ptrbuf != ptrend )
  {
    auto  cbread = recv( sockFd, ptrbuf, ptrend - ptrbuf, MSG_DONTWAIT | MSG_NOSIGNAL );
    int   nerror;

    if ( cbread > 0 )
    {
      ptrbuf += cbread;
      continue;
    }
    if ( cbread == 0 )
    {
      close( sockFd );
        sockFd = -1;
      break;
    }
    if ( (nerror = errno) != EAGAIN && nerror != EWOULDBLOCK )
    {
      close( sockFd );
      sockFd = -1;
    }
    break;
  }
  return sockFd == -1 ? uint32_t(-1) : ptrbuf - (char*)pv;
}

uint32_t  ChunkedStream::Put( const void* pv, uint32_t cc )
{
  auto  ptrbuf = static_cast<const char*>( pv );
  auto  ptrend = static_cast<const char*>( pv ) + cc;

  while ( ptrbuf != ptrend )
  {
  // copy possible part of data
    while ( ptrbuf != ptrend && bufptr != std::end(buffer) )
      *bufptr++ = *ptrbuf++;

  // check if the buffer is full and may be sent to socket
  // copy the part to be sent
    if ( bufptr == std::end(buffer) )
    {
      const char  header[] = "400\r\n";

      if ( put( header, 5 ) < 0 )
      {
        close( sockFd );
        return uint32_t(-1);
      }
      if ( put( buffer, bufptr - buffer ) < 0 )
      {
        close( sockFd );
        return uint32_t(-1);
      }
      if ( put( "\r\n", 2 ) < 0 )
      {
        close( sockFd );
        return uint32_t(-1);
      }
      bufptr = buffer;
    }
  }
  return cc;
}

int   ChunkedStream::put( const void* pv, uint32_t cc )
{
  auto  ptrbuf = static_cast<const char*>( pv );
  auto  ptrend = static_cast<const char*>( pv ) + cc;

  while ( ptrbuf != ptrend )
  {
    auto  nwrite = send( sockFd, ptrbuf, ptrend - ptrbuf, MSG_DONTWAIT | MSG_NOSIGNAL );

    if ( nwrite >= 0 )  ptrbuf += nwrite;
      else return nwrite;
  }
  return cc;
}

auto  Request( const http::Request& req ) -> mtc::api<mtc::IByteStream>
{

}
