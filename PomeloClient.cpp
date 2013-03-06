#include "PomeloClient.h"
#include <string>

using namespace std;
using namespace PomeloCpp;

class Protocal
{
public:
	static std::string encode( int requestID, const char* route, Json::Value* jsonObject )
	{
		string result;
		result.push_back( (char)( (requestID >> 24) & 0xff) );
		result.push_back( (char)( (requestID >> 16) & 0xff) );
		result.push_back( (char)( (requestID >> 8) & 0xff) );
		result.push_back( (char)( requestID & 0xff) );

		size_t routeLen = strlen( route );
		if ( routeLen > 255 )
			throw string( "route name exceed 255!" ) + route;

		result.push_back( (char)routeLen );
		
		Json::FastWriter writer;
		result += writer.write( *jsonObject );

		return result;
	}
};

PomeloClient::PomeloClient(void)
	: m_requestId(0)
	, m_connection( NULL )
{
}

PomeloClient::~PomeloClient(void)
{
	if ( m_connection )
	{
		m_connection->removeHandler( this );	
		disconnect();			
	}
}

PomeloCpp::PomeloClient::PomeloClient( const char* host, unsigned short port )
	: m_requestId(0)
{
	if ( !init( host, port ) )
		throw "PomeloClient init failed";
}

bool PomeloCpp::PomeloClient::init( const char* host, unsigned short port )
{
	m_connection = new Private::Socket( host, port );
	if ( m_connection )
		m_connection->addHandler( this );

	return true;
}

void PomeloCpp::PomeloClient::request( const char* route, Json::Value* msg /*= NULL */, MsgHandler callback /*= NULLL */ )
{
	++m_requestId;
	if ( m_requestId == 0 )
		++m_requestId;

	string msgBuffer = Protocal::encode( m_requestId, route, msg );
	sendMesage( msgBuffer.c_str(), msgBuffer.length() );

	m_responseHandlers[ m_requestId ].push_back( callback );
}

void PomeloCpp::PomeloClient::notify( const char* route, Json::Value* msg /*= NULL */ )
{
	request( route, msg );
}

void PomeloCpp::PomeloClient::registerMsgHandler( const char* route, MsgHandler hanlder )
{
	m_broadcastHandlers[ route ].push_back( hanlder );
}

void PomeloCpp::PomeloClient::sendMesage( const char* msg, size_t len )
{
	if ( m_connection && m_connection->isConnected() )
		m_connection->sendData( msg, len );
}

void PomeloCpp::PomeloClient::receiveMessage( const char* msg, size_t len )
{
	MsgHandlerList* handlers = NULL;
	Json::Value msgObject( msg, msg + len );
	
	int requestId = 0;
	if ( msgObject["id"] != NULL )
	{		
		requestId = msgObject["id"].asInt();
		ResponceMsgHandlerMap::iterator iHandler = m_responseHandlers.find( requestId );
		if ( iHandler != m_responseHandlers.end() )
		{
			handlers = &iHandler->second;
		}
	}	
	
	const char* route = msgObject["route"].asCString();

	if ( !handlers )
	{
		BroadcastMsgHandlerMap::iterator iHandler = m_broadcastHandlers.find( route );
		if ( iHandler != m_broadcastHandlers.end() )
		{
			handlers = &iHandler->second;
		}
	}	

	if ( handlers )
	{
		for ( size_t i = 0; i < handlers->size(); ++i )
		{
			MsgHandler& handler = handlers->at(i);
			cout << msgObject["body"] << endl;
			//handler( msgObject["body"] );
		}
	}

	if ( requestId != 0 )
	{
		m_responseHandlers.erase( requestId );
	}
}

void PomeloCpp::PomeloClient::updateStatus()
{
	m_connection->updateStatus();
}

void PomeloCpp::PomeloClient::onReceiveData( const void* pData, size_t size )
{
	receiveMessage( (const char*)pData, size );
}

void PomeloCpp::PomeloClient::onConnected()
{
	Json::Value msg;
	msg.clear();
	msg["uid"] = "xophiix";
	request( "gate.gateHandler.queryEntry", &msg );
}

void PomeloCpp::PomeloClient::onDisconnected()
{

}

bool PomeloCpp::PomeloClient::connected() const
{
	return m_connection && m_connection->isConnected();
}

void PomeloCpp::PomeloClient::disconnect()
{
	if ( m_connection )
	{
		m_connection->disconnect();
		delete m_connection;
		m_connection = NULL;
	}
}
