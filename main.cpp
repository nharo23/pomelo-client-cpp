#include "PomeloClient.h"

using namespace PomeloCpp;

int main( int argc, char** argv )
{
	PomeloClient client( "127.0.0.1", 3014 );
	
	while( client.connectionValid() )
	{
		client.updateStatus();
	}

	return 0;
}