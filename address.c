/** 
 @file list.c
 @brief ENet linked list functions
*/
#define ENET_BUILDING_LIB 1
#include "enet/enet.h"

#include <stdio.h>
#include <string.h>

// http://rosettacode.org/wiki/Parse_an_IP_Address#C

static unsigned int _parseDecimal ( const char** pchCursor )
{
    unsigned int nVal = 0;
    char chNow;
    while ( chNow = **pchCursor, chNow >= '0' && chNow <= '9' )
    {
        //shift digit in
        nVal *= 10;
        nVal += chNow - '0';
 
        ++*pchCursor;
    }
    return nVal;
}
 
 
 
static unsigned int _parseHex ( const char** pchCursor )
{
    unsigned int nVal = 0;
    char chNow;
    while ( chNow = **pchCursor & 0x5f, //(collapses case, but mutilates digits)
            (chNow >= ('0'&0x5f) && chNow <= ('9'&0x5f)) || 
            (chNow >= 'A' && chNow <= 'F') 
            )
    {
        unsigned char nybbleValue;
        chNow -= 0x10;  //scootch digital values down; hex now offset by x31
        nybbleValue = ( chNow > 9 ? chNow - (0x31-0x0a) : chNow );
        //shift nybble in
        nVal <<= 4;
        nVal += nybbleValue;
 
        ++*pchCursor;
    }
    return nVal;
}
 
 
 
//Parse a textual IPv4 or IPv6 address, optionally with port, into a binary
//array (for the address, in network order), and an optionally provided port.
//Also, indicate which of those forms (4 or 6) was parsed.  Return true on
//success.  ppszText must be a nul-terminated ASCII string.  It will be
//updated to point to the character which terminated parsing (so you can carry
//on with other things.  abyAddr must be 16 bytes.  You can provide NULL for
//abyAddr, nPort, bIsIPv6, if you are not interested in any of those
//informations.  If we request port, but there is no port part, then nPort will
//be set to 0.  There may be no whitespace leading or internal (though this may
//be used to terminate a successful parse.
//Note:  the binary address and integer port are in network order.
static int ParseIPv4OrIPv6 ( const char** ppszText, enet_uint8* abyAddr, enet_uint16* pnPort, int* pbIsIPv6 )
{
    unsigned char* abyAddrLocal;
    unsigned char abyDummyAddr[16];
 
    //find first colon, dot, and open bracket
    const char* pchColon = strchr ( *ppszText, ':' );
    const char* pchDot = strchr ( *ppszText, '.' );
    const char* pchOpenBracket = strchr ( *ppszText, '[' );
    const char* pchCloseBracket = NULL;
 
 
    //we'll consider this to (probably) be IPv6 if we find an open
    //bracket, or an absence of dots, or if there is a colon, and it
    //precedes any dots that may or may not be there
    int bIsIPv6local = NULL != pchOpenBracket || NULL == pchDot ||
            ( NULL != pchColon && ( NULL == pchDot || pchColon < pchDot ) );
    //OK, now do a little further sanity check our initial guess...
    if ( bIsIPv6local )
    {
        //if open bracket, then must have close bracket that follows somewhere
        pchCloseBracket = strchr ( *ppszText, ']' );
        if ( NULL != pchOpenBracket && ( NULL == pchCloseBracket ||
                pchCloseBracket < pchOpenBracket ) )
            return 0;
    }
    else    //probably ipv4
    {
        //dots must exist, and precede any colons
        if ( NULL == pchDot || ( NULL != pchColon && pchColon < pchDot ) )
            return 0;
    }
 
    //we figured out this much so far....
    if ( NULL != pbIsIPv6 )
        *pbIsIPv6 = bIsIPv6local;
 
    //especially for IPv6 (where we will be decompressing and validating)
    //we really need to have a working buffer even if the caller didn't
    //care about the results.
    abyAddrLocal = abyAddr; //prefer to use the caller's
    if ( NULL == abyAddrLocal ) //but use a dummy if we must
        abyAddrLocal = abyDummyAddr;
 
    //OK, there should be no correctly formed strings which are miscategorized,
    //and now any format errors will be found out as we continue parsing
    //according to plan.
    if ( ! bIsIPv6local )   //try to parse as IPv4
    {
        //4 dotted quad decimal; optional port if there is a colon
        //since there are just 4, and because the last one can be terminated
        //differently, I'm just going to unroll any potential loop.
        unsigned char* pbyAddrCursor = abyAddrLocal;
        unsigned int nVal;
        const char* pszTextBefore = *ppszText;
        nVal =_parseDecimal ( ppszText );           //get first val
        if ( '.' != **ppszText || nVal > 255 || pszTextBefore == *ppszText )    //must be in range and followed by dot and nonempty
            return 0;
        *(pbyAddrCursor++) = (unsigned char) nVal;  //stick it in addr
        ++(*ppszText);  //past the dot
 
        pszTextBefore = *ppszText;
        nVal =_parseDecimal ( ppszText );           //get second val
        if ( '.' != **ppszText || nVal > 255 || pszTextBefore == *ppszText )
            return 0;
        *(pbyAddrCursor++) = (unsigned char) nVal;
        ++(*ppszText);  //past the dot
 
        pszTextBefore = *ppszText;
        nVal =_parseDecimal ( ppszText );           //get third val
        if ( '.' != **ppszText || nVal > 255 || pszTextBefore == *ppszText )
            return 0;
        *(pbyAddrCursor++) = (unsigned char) nVal;
        ++(*ppszText);  //past the dot
 
        pszTextBefore = *ppszText;
        nVal =_parseDecimal ( ppszText );           //get fourth val
        if ( nVal > 255 || pszTextBefore == *ppszText ) //(we can terminate this one in several ways)
            return 0;
        *(pbyAddrCursor++) = (unsigned char) nVal;
 
        if ( ':' == **ppszText && NULL != pnPort )  //have port part, and we want it
        {
            ++(*ppszText);  //past the colon
            pszTextBefore = *ppszText;
            nVal =_parseDecimal ( ppszText );
            if ( nVal > 65535 || pszTextBefore == *ppszText )
                return 0;

            *pnPort = (enet_uint16) nVal;
            return 1;
        }
        else    //finished just with ip address
        {
            if ( NULL != pnPort )
                *pnPort = 0;    //indicate we have no port part
            return 1;
        }
    }
    else    //try to parse as IPv6
    {
        unsigned char* pbyAddrCursor;
        unsigned char* pbyZerosLoc;
        int bIPv4Detected;
        int nIdx;
        //up to 8 16-bit hex quantities, separated by colons, with at most one
        //empty quantity, acting as a stretchy run of zeroes.  optional port
        //if there are brackets followed by colon and decimal port number.
        //A further form allows an ipv4 dotted quad instead of the last two
        //16-bit quantities, but only if in the ipv4 space ::ffff:x:x .
        if ( NULL != pchOpenBracket )   //start past the open bracket, if it exists
            *ppszText = pchOpenBracket + 1;
        pbyAddrCursor = abyAddrLocal;
        pbyZerosLoc = NULL; //if we find a 'zero compression' location
        bIPv4Detected = 0;
        for ( nIdx = 0; nIdx < 8; ++nIdx )  //we've got up to 8 of these, so we will use a loop
        {
            const char* pszTextBefore = *ppszText;
            unsigned nVal =_parseHex ( ppszText );      //get value; these are hex
            if ( pszTextBefore == *ppszText )   //if empty, we are zero compressing; note the loc
            {
                if ( NULL != pbyZerosLoc )  //there can be only one!
                {
                    //unless it's a terminal empty field, then this is OK, it just means we're done with the host part
                    if ( pbyZerosLoc == pbyAddrCursor )
                    {
                        --nIdx;
                        break;
                    }
                    return 0;   //otherwise, it's a format error
                }
                if ( ':' != **ppszText )    //empty field can only be via :
                    return 0;
                if ( 0 == nIdx )    //leading zero compression requires an extra peek, and adjustment
                {
                    ++(*ppszText);
                    if ( ':' != **ppszText )
                        return 0;
                }
 
                pbyZerosLoc = pbyAddrCursor;
                ++(*ppszText);
            }
            else
            {
                if ( '.' == **ppszText )    //special case of ipv4 convenience notation
                {
                    //who knows how to parse ipv4?  we do!
                    const char* pszTextlocal = pszTextBefore;   //back it up
                    unsigned char abyAddrlocal[16];
                    int bIsIPv6local;
                    int bParseResultlocal = ParseIPv4OrIPv6 ( &pszTextlocal, abyAddrlocal, NULL, &bIsIPv6local );
                    *ppszText = pszTextlocal;   //success or fail, remember the terminating char
                    if ( ! bParseResultlocal || bIsIPv6local )  //must parse and must be ipv4
                        return 0;
                    //transfer addrlocal into the present location
                    *(pbyAddrCursor++) = abyAddrlocal[0];
                    *(pbyAddrCursor++) = abyAddrlocal[1];
                    *(pbyAddrCursor++) = abyAddrlocal[2];
                    *(pbyAddrCursor++) = abyAddrlocal[3];
                    ++nIdx; //pretend like we took another short, since the ipv4 effectively is two shorts
                    bIPv4Detected = 1;  //remember how we got here for further validation later
                    break;  //totally done with address
                }
 
                if ( nVal > 65535 ) //must be 16 bit quantity
                    return 0;
                *(pbyAddrCursor++) = nVal >> 8;     //transfer in network order
                *(pbyAddrCursor++) = nVal & 0xff;
                if ( ':' == **ppszText )    //typical case inside; carry on
                {
                    ++(*ppszText);
                }
                else    //some other terminating character; done with this parsing parts
                {
                    break;
                }
            }
        }
 
        //handle any zero compression we found
        if ( NULL != pbyZerosLoc )
        {
            int nHead = (int)( pbyZerosLoc - abyAddrLocal );    //how much before zero compression
            int nTail = nIdx * 2 - (int)( pbyZerosLoc - abyAddrLocal ); //how much after zero compression
            int nZeros = 16 - nTail - nHead;        //how much zeros
            memmove ( &abyAddrLocal[16-nTail], pbyZerosLoc, nTail );    //scootch stuff down
            memset ( pbyZerosLoc, 0, nZeros );      //clear the compressed zeros
        }
 
        //validation of ipv4 subspace ::ffff:x.x
        if ( bIPv4Detected )
        {
            static const unsigned char abyPfx[] = { 0,0, 0,0, 0,0, 0,0, 0,0, 0xff,0xff };
            if ( 0 != memcmp ( abyAddrLocal, abyPfx, sizeof(abyPfx) ) )
                return 0;
        }
 
        //close bracket
        if ( NULL != pchOpenBracket )
        {
            if ( ']' != **ppszText )
                return 0;
            ++(*ppszText);
        }
 
        if ( ':' == **ppszText && NULL != pnPort )  //have port part, and we want it
        {
            const char* pszTextBefore;
            unsigned int nVal;
            ++(*ppszText);  //past the colon
            pszTextBefore = *ppszText;
            pszTextBefore = *ppszText;
            nVal =_parseDecimal ( ppszText );
            if ( nVal > 65535 || pszTextBefore == *ppszText )
                return 0;

            *pnPort = (enet_uint16) nVal;
            return 1;
        }
        else    //finished just with ip address
        {
            if ( NULL != pnPort )
                *pnPort = 0;    //indicate we have no port part
            return 1;
        }
    }
 
}

int enet_address_equal_host(const ENetAddress * firstAddress, const ENetAddress * secondAddress)
{
    if (firstAddress->type != secondAddress->type)
        return 0;

    if (firstAddress->port != secondAddress->port)
        return 0;

    if (firstAddress->type == ENET_ADDRESS_TYPE_IPV4)
    {
        if (memcmp(&firstAddress->host.v4[0], &secondAddress->host.v4[0], 4*sizeof(enet_uint8)) != 0)
            return 0;
    }
    else if (firstAddress->type == ENET_ADDRESS_TYPE_IPV6)
    {
        if (memcmp(&firstAddress->host.v6[0], &secondAddress->host.v6[0], 8 * sizeof(enet_uint16)) != 0)
            return 0;
    }

    return 1;
}

int enet_address_equal(const ENetAddress * firstAddress, const ENetAddress * secondAddress)
{
    if (!enet_address_equal_host(firstAddress, secondAddress))
        return 0;

    if (firstAddress->port != secondAddress->port)
        return 0;

    return 1;
}

int 
enet_address_is_any(const ENetAddress * address)
{
    switch (address->type)
    {
        case ENET_ADDRESS_TYPE_IPV4:
        {
            enet_uint8 zero[4] = { 0 };
            return memcmp(&address->host.v4[0], zero, 4 * sizeof(enet_uint8)) != 0;
        }

        case ENET_ADDRESS_TYPE_IPV6:
        {
            enet_uint16 zero[8] = { 0 };
            return memcmp(&address->host.v6[0], zero, 8 * sizeof(enet_uint16)) != 0;
        }

        default:
            break;
    }

    return 0;
}

int 
enet_address_is_broadcast(const ENetAddress * address)
{
    /* there's no broadcast address in ipv6 */
    if (address->type == ENET_ADDRESS_TYPE_IPV4 &&
        address->host.v4[0] == 0xFF &&
        address->host.v4[1] == 0xFF &&
        address->host.v4[2] == 0xFF &&
        address->host.v4[3] == 0xFF)
    {
        return 1;
    }

    return 0;
}

int 
enet_address_is_loopback(const ENetAddress * address)
{
    switch (address->type)
    {
        case ENET_ADDRESS_TYPE_IPV4:
            return address->host.v4[0] == 127;

        case ENET_ADDRESS_TYPE_IPV6:
        {
            enet_uint16 loopback[8] = { 0, 0, 0, 0, 0, 0, 0, 1 };
            return memcmp(&address->host.v6[0], loopback, 8 * sizeof(enet_uint16)) != 0;
        }

        default:
            break;
    }

    return 0;
}

int
enet_address_get_host_ip (const ENetAddress * address, char * name, size_t nameLength)
{
    enum MaxSize
    { 
        MS_PortPart = 1 + 4,                    /* semicolon + 4 digits */

        MS_IPv4 = 4 * 3 + 3 + 1,                /* 4 * 3 digits plus three dots plus end of string character */
        MS_IPv4MappedIPv6 = 7 + 4 * 3 + 3 + 1,  /* prefix + 4 * 3 digits plus three dots plus end of string character */
        MS_IPv6 = 8 * 4 + 7 + 1,                /* 4 * 3 digits plus three dots plus end of string character */

        MS_IPv4WithPort = MS_IPv4 + MS_PortPart,
        MS_IPv4MappedIPv6WithPort = MS_IPv4MappedIPv6 + MS_PortPart + 2, /* plus two brackets */
        MS_IPv6WithPort = MS_IPv6 + MS_PortPart + 2,                     /* plus two brackets */
    };

    if (address -> type == ENET_ADDRESS_TYPE_IPV4)
    {
        if (address->port != 0)
        {
            if (nameLength < MS_IPv4WithPort)
                return -1;

            sprintf(name, "%u.%u.%u.%u:%u", address->host.v4[0], address->host.v4[1], address->host.v4[2], address->host.v4[3], address->port);
        }
        else
        {
            if (nameLength < MS_IPv4)
                return -1;

            sprintf(name, "%u.%u.%u.%u", address->host.v4[0], address->host.v4[1], address->host.v4[2], address->host.v4[3]);
        }
    }
    else if (address->type == ENET_ADDRESS_TYPE_IPV6)
    {
        /* check if ipv4-mapped address */
        if (address -> host.v6[0] == 0 && address -> host.v6[1] == 0 &&
            address -> host.v6[2] == 0 && address -> host.v6[3] == 0 &&
            address -> host.v6[4] == 0 && address -> host.v6[5] == 0xFFFF)
        {
            if (address->port != 0)
            {
                if (nameLength < MS_IPv4MappedIPv6WithPort)
                    return -1;

                sprintf(name, "[::ffff:%u.%u.%u.%u]:%u",
                        (address->host.v6[6] >> 8) & 0xFF, (address->host.v6[6] >> 0) & 0xFF,
                        (address->host.v6[7] >> 8) & 0xFF, (address->host.v6[7] >> 0) & 0xFF, 
                        address->port);
            }
            else
            {
                if (nameLength < MS_IPv4MappedIPv6)
                    return -1;

                sprintf(name, "::ffff:%u.%u.%u.%u",
                        (address->host.v6[6] >> 8) & 0xFF, (address->host.v6[6] >> 0) & 0xFF,
                        (address->host.v6[7] >> 8) & 0xFF, (address->host.v6[7] >> 0) & 0xFF);
            }
        }
        else
        {
            /* standard IPv6 
             * canonical representation of an IPv6
             * https://tools.ietf.org/html/rfc5952
            */

            const size_t maxSize = 8 * 4 + 7 + 1; /* 4 * 3 digits plus three dots plus end of string character */
            char buffer[MS_IPv6WithPort];

            /* find the longest zero sequence */
            int f0 = -1;
            int l0 = -1;
            int i, j;
            char* p = buffer;
            int advance;

            for (i = 0; i < 8; ++i)
            {
                if (address -> host.v6[i] == 0)
                {
                    for (j = i + 1; j < 8; ++j)
                    {
                        if (address -> host.v6[j] != 0)
                            break;
                    }

                    if (f0 == -1 || j - i > l0 - f0)
                    {
                        f0 = i;
                        l0 = j;
                    }
                }
            }

            /* We need brackets around our IPv6 address if we have a port */
            if (address->port != 0)
                *p++ = '[';

            for (i = 0; i < 8; ++i)
            {
                if (i == f0)
                {
                    *p++ = ':';
                    *p++ = ':';

                    i = l0;
                    if (i >= 8)
                        break;
                }
                else if (i != 0)
                    *p++ = ':';

                advance = sprintf(p, "%x", address -> host.v6[i]);
                p += advance;
            }

            if (address->port != 0)
            {
                *p++ = ']';
                *p++ = ':';

                advance = sprintf(p, "%u", address->port);
                p += advance;
            }

            *p = '\0';

            size_t addrLen = p - buffer;
            if (nameLength < addrLen)
                return -1;

            memcpy(name, buffer, addrLen);
        }
    }

    return 0;
}


int
enet_address_set_host_ip (ENetAddress * address, const char * name)
{
    enet_uint8 result[16];
    int isIPv6;
    int i;
    enet_uint16 port;

    if (!ParseIPv4OrIPv6(&name, result, &port, &isIPv6))
        return -1;

    if (isIPv6)
    {
        address->type = ENET_ADDRESS_TYPE_IPV6;

        for (i = 0; i < 8; ++i)
            address->host.v6[i] = (result[i * 2] << 8) | result[i * 2 + 1];
    }
    else
    {
        address->type = ENET_ADDRESS_TYPE_IPV4;

        for (i = 0; i < 4; ++i)
            address->host.v4[i] = result[i];
    }

    if (port)
        address->port = port;

    return 0;
}


void enet_address_build_any(ENetAddress * address, ENetAddressType type)
{
    address->type = type;

    switch (type)
    {
        case ENET_ADDRESS_TYPE_IPV4:
        {
            memset(&address->host.v4, 0, sizeof(address->host.v4));
            address->port = ENET_PORT_ANY;
            break;
        }

        case ENET_ADDRESS_TYPE_IPV6:
        {
            memset(&address->host.v6, 0, sizeof(address->host.v6));
            address->port = ENET_PORT_ANY;
            break;
        }

        default:
            break;
    }
}

void enet_address_build_loopback(ENetAddress * address, ENetAddressType type)
{
    address->type = type;

    switch (type)
    {
        case ENET_ADDRESS_TYPE_IPV4:
        {
            enet_uint8 loopbackIpV4[4] = { 127, 0, 0, 1 };

            memcpy(&address->host.v4, loopbackIpV4, sizeof(address->host.v4));
            address->port = ENET_PORT_ANY;
            break;
        }

        case ENET_ADDRESS_TYPE_IPV6:
        {
            enet_uint16 loopbackIpV6[8] = { 0, 0, 0, 0, 0, 0, 0, 1 };

            memcpy(&address->host.v6, loopbackIpV6, sizeof(address->host.v6));
            address->port = ENET_PORT_ANY;
            break;
        }

        default:
            break;
    }
}

void enet_address_convert_ipv6(ENetAddress * address)
{
    enet_uint8 ipv4[4];

    if (address->type == ENET_ADDRESS_TYPE_IPV4)
    {
        /* Turn into IPv4-mapped IPv6 */
        address->type = ENET_ADDRESS_TYPE_IPV6;
        memcpy(&ipv4[0], &address->host.v4[0], 4 * sizeof(enet_uint8)); /* save IPv4 before modifying the IPv6 because of the union */

        address->host.v6[0] = 0;
        address->host.v6[1] = 0;
        address->host.v6[2] = 0;
        address->host.v6[3] = 0;
        address->host.v6[4] = 0;
        address->host.v6[5] = 0xFFFF;
        address->host.v6[6] = ((enet_uint16) ipv4[0]) << 8 | ipv4[1];
        address->host.v6[7] = ((enet_uint16) ipv4[2]) << 8 | ipv4[3];
    }
}


/** @} */
