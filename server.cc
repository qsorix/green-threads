/*
 * Let's what APIs we need to create a server.
 */
void read_request()
{
	while (reading_request)
	{
		mt::yield_select(client)
		read_request(client)
	}
}

void do_sth_with_database()
{
	query q = "select ...";
	database.execute(q);
	mt::yield_select(database); // a common wrapper on I/O objects would be nice
	result = database.result();
}

void write_response()
{
	while (writing_response)
	{
		mt::yield_select(client);
		write_response(client);
	}
}

void handle_connection(socket client)
{
	read_request();

	do_sth_with_database();

	write_request();
}

void server()
{
	socket server_socket;
	while(true)
	{
		// this requires more sophisticated data structure to hold
		// tasks, plus a smarter scheduler. i.e. we wan't to active such
		// tasks only when select tells they have something to do and
		// skip them otherwise.
		mt::yield_select(server_socket);

		socket client = accept(server_socket);

		// right now tart sucks at passing arguments so this can't be
		// done
		start(boost::bind(handle_connection, socket_client));
	}
}

int main()
{
	mt::execute(server);
}
