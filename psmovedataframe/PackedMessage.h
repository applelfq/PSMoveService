//
// packedmessage.h: packaging of messages into length-prepended buffers
// ready for transmission.
//
// Eli Bendersky (eliben@gmail.com)
// This code is in the public domain
//
#ifndef PACKEDMESSAGE_H
#define PACKEDMESSAGE_H

#include <string>
#include <cassert>
#include <vector>
#include <cstdio>
#include <memory>
#include <boost/cstdint.hpp>


typedef std::vector<boost::uint8_t> data_buffer;


// A generic function to show contents of a container holding byte data 
// as a string with hex representation for each byte.
//
template <class CharContainer>
std::string show_hex(const CharContainer& c)
{
    std::string hex;
    char buf[16];
    typename CharContainer::const_iterator i;
    for (i = c.begin(); i != c.end(); ++i) {
        std::sprintf(buf, "%02X ", static_cast<unsigned>(*i) & 0xFF);
        hex += buf;
    }
    return hex;
}

std::string show_hex(const uint8_t * c, unsigned length)
{
    std::string hex;
    char buf[16];
    for (unsigned i = 0; i < length; ++i) 
    {
        std::sprintf(buf, "%02X ", static_cast<unsigned>(c[i]) & 0xFF);
        hex += buf;
    }
    return hex;
}

// The header size for packed messages
//
const unsigned HEADER_SIZE = 4;


// PackedMessage implements simple "packing" of protocol buffers Messages into
// a string prepended by a header specifying the message length.
// MessageType should be a Message class generated by the protobuf compiler.
//
template <class MessageType>
class PackedMessage 
{
public:
    typedef std::shared_ptr<MessageType> MessagePointer;

    PackedMessage(MessagePointer msg = MessagePointer())
        : m_msg(msg)
    {}

    void set_msg(MessagePointer msg)
    {
        m_msg = msg;
    }

    MessagePointer get_msg()
    {
        return m_msg;
    }

    // Pack the message into the given data_buffer. The buffer is resized to
    // exactly fit the message.
    // Return false in case of an error, true if successful.
    //
    bool pack(data_buffer& buf) const
    {
        if (!m_msg)
            return false;

        unsigned msg_size = m_msg->ByteSize();
        buf.resize(HEADER_SIZE + msg_size);

        encode_header(buf, msg_size);

        if (msg_size > 0)
        {
            return m_msg->SerializeToArray(&buf[HEADER_SIZE], msg_size);
        }
        else
        {
            // no body to encode (i.e. just using message defaults)
            return true;
        }
    }

    // Pack the message into the given fixed sized data_buffer. 
    // The message must fit inside the given array
    // Return false in case of an error, true if successful.
    //
    bool pack(boost::uint8_t *buf, int buf_size) const
    {
        if (!m_msg)
            return false;

        int msg_size = m_msg->ByteSize();
        if ((int)HEADER_SIZE + msg_size < buf_size)
        {
            memset(buf, 0, buf_size);
            encode_header(buf, buf_size, msg_size);

            if (msg_size > 0)
            {
                return m_msg->SerializeToArray(&buf[HEADER_SIZE], msg_size);
            }
            else
            {
                // no body to encode (i.e. just using message defaults)
                return true;
            }
        }
        else
        {
            return false;
        }
    }

    // Given a buffer with the first HEADER_SIZE bytes representing the header,
    // decode the header and return the message length. Return 0 in case of 
    // an error.
    //
    unsigned decode_header(const data_buffer& buf) const
    {
        if (buf.size() < HEADER_SIZE)
            return 0;
        unsigned msg_size = 0;
        for (unsigned i = 0; i < HEADER_SIZE; ++i)
            msg_size = msg_size * 256 + (static_cast<unsigned>(buf[i]) & 0xFF);
        return msg_size;
    }

    // Given a buffer with the first HEADER_SIZE bytes representing the header,
    // decode the header and return the message length. Return 0 in case of 
    // an error.
    //
    unsigned decode_header(const boost::uint8_t *buf, unsigned int buf_size) const
    {
        if (buf_size < HEADER_SIZE)
            return 0;
        unsigned msg_size = 0;
        for (unsigned i = 0; i < HEADER_SIZE; ++i)
            msg_size = msg_size * 256 + (static_cast<unsigned>(buf[i]) & 0xFF);
        return msg_size;
    }

    // Unpack and store a message from the given packed buffer.
    // Return true if unpacking successful, false otherwise.
    //
    bool unpack(const data_buffer& buf)
    {
        m_msg->Clear();

        if (buf.size() - HEADER_SIZE > 0)
        {
            return m_msg->ParseFromArray(&buf[HEADER_SIZE], buf.size() - HEADER_SIZE);
        }
        else
        {
            // no body to decode (i.e. just using message defaults)
            return true;
        }
    }

    // Unpack and store a message from the given packed buffer.
    // Return true if unpacking successful, false otherwise.
    //
    bool unpack(const boost::uint8_t *buf, unsigned buf_size)
    {
        m_msg->Clear();

        if (buf_size - HEADER_SIZE > 0)
        {
            return m_msg->ParseFromArray(&buf[HEADER_SIZE], buf_size - HEADER_SIZE);
        }
        else
        {
            // no body to decode (i.e. just using message defaults)
            return true;
        }
    }

private:
    // Encodes the size into a header at the beginning of buf
    //
    void encode_header(data_buffer& buf, unsigned value) const
    {
        assert(buf.size() >= HEADER_SIZE);
        buf[0] = static_cast<boost::uint8_t>((value >> 24) & 0xFF);
        buf[1] = static_cast<boost::uint8_t>((value >> 16) & 0xFF);
        buf[2] = static_cast<boost::uint8_t>((value >> 8) & 0xFF);
        buf[3] = static_cast<boost::uint8_t>(value & 0xFF);
    }

    void encode_header(boost::uint8_t *buf, unsigned buf_size, unsigned value) const
    {
        assert(buf_size >= HEADER_SIZE);
        buf[0] = static_cast<boost::uint8_t>((value >> 24) & 0xFF);
        buf[1] = static_cast<boost::uint8_t>((value >> 16) & 0xFF);
        buf[2] = static_cast<boost::uint8_t>((value >> 8) & 0xFF);
        buf[3] = static_cast<boost::uint8_t>(value & 0xFF);
    }

    MessagePointer m_msg;
};

#endif /* PACKEDMESSAGE_H */

