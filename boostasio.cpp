#include "boostasio.hpp"


redisBoostClient::redisBoostClient(boost::asio::io_service& io_service,redisAsyncContext *ac)
: socket_(io_service)
{

	/*this gives us access to c->fd*/
	redisContext *c = &(ac->c);

	/*pass this to hiredis hooks*/	
	context_ = ac;		

	/*hiredis already connected
	 *use the existing native socket
	 */
	socket_.assign(boost::asio::ip::tcp::v4(),c->fd);

	/*register hooks with the hiredis async context*/
	ac->ev.addRead = call_C_addRead;
	ac->ev.delRead = call_C_delRead;
	ac->ev.addWrite = call_C_addWrite;
	ac->ev.delWrite = call_C_delWrite;
	ac->ev.cleanup = call_C_cleanup;

	/*I don't know whether this even gets used, but...*/
	ac->ev.data = ac;

	/*C wrapper functions will use this
	 *pointer to call class members.
	 *fortunately hiredis doesn't use the data field.
	 *see line 70 of async.h
	 */
	ac->data = this;
}

void redisBoostClient::operate()
{
	if(read_requested_ && !read_in_progress_) {
		read_in_progress_ = true;
		socket_.async_read_some(boost::asio::null_buffers(),
                       	boost::bind(&redisBoostClient::handle_read,this,boost::asio::placeholders::error));
	}

	if(write_requested_ && !write_in_progress_) {
		write_in_progress_ = true;
		socket_.async_write_some(boost::asio::null_buffers(),
                       	boost::bind(&redisBoostClient::handle_write,this,boost::asio::placeholders::error));
	}
}
	
void redisBoostClient::handle_read(boost::system::error_code ec)
{
	read_in_progress_ = false;
	if(!ec) {
		redisAsyncHandleRead(context_);
	}

        if (!ec || ec == boost::asio::error::would_block)
	        operate();
}

void redisBoostClient::handle_write(boost::system::error_code ec)
{
	write_in_progress_ = false;
	if(!ec) {
		redisAsyncHandleWrite(context_);
	}

	if (!ec || ec == boost::asio::error::would_block)
		operate();
}

void redisBoostClient::add_read(void *privdata) 
{
	read_requested_ = true;
	operate();
}

void redisBoostClient::del_read(void *privdata) 
{
	read_requested_ = false;
}

void redisBoostClient::add_write(void *privdata) 
{
	write_requested_ = true;
	operate();
}

void redisBoostClient::del_write(void *privdata) 
{
	write_requested_ = false;
}

void redisBoostClient::cleanup(void *privdata) 
{
	/*Do I even need this?*/
	printf("cleanup called...\n");	
}

/*wrappers*/
extern "C" void call_C_addRead(void *privdata)
{
	redisAsyncContext *ac = (redisAsyncContext*)privdata;
	redisBoostClient *C = (redisBoostClient*)ac->data;
	C->add_read(privdata);
}

extern "C" void call_C_delRead(void *privdata)
{
        redisAsyncContext *ac = (redisAsyncContext*)privdata;
        redisBoostClient *C = (redisBoostClient*)ac->data;
        C->del_read(privdata);
}

extern "C" void call_C_addWrite(void *privdata)
{
        redisAsyncContext *ac = (redisAsyncContext*)privdata;
        redisBoostClient *C = (redisBoostClient*)ac->data;
        C->add_write(privdata);
}

extern "C" void call_C_delWrite(void *privdata)
{
        redisAsyncContext *ac = (redisAsyncContext*)privdata;
        redisBoostClient *C = (redisBoostClient*)ac->data;
        C->del_write(privdata);
}

extern "C" void call_C_cleanup(void *privdata)
{
        redisAsyncContext *ac = (redisAsyncContext*)privdata;
        redisBoostClient *C = (redisBoostClient*)ac->data;
        C->cleanup(privdata);
}
/*end of wrappers*/


