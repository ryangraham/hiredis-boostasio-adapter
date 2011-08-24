#include "boostasio.hpp"

void getCallback(redisAsyncContext *c, void *r, void *privdata) {
    redisReply *reply = (redisReply*)r;
    if (reply == NULL) return;
    printf("argv[%s]: %s\n", (char*)privdata, reply->str);

    /* Disconnect after receiving the reply to GET */
    redisAsyncDisconnect(c);
}

void connectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        return;
    }
    printf("Connected...\n");
}

void disconnectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        return;
    }
    printf("Disconnected...\n");
}

int main(int argc,char **argv)
{
        /*ask hiredis to connect to redis*/
        redisAsyncContext *ac = redisAsyncConnect("127.0.0.1", 6379);
                if(ac->err) {
                        /* Let *ac leak for now... */
                        printf("Error: %s\n", ac->errstr);
                        return 1;
                }

        boost::asio::io_service io_service;

        /*jack in*/
        redisBoostClient client(io_service,ac);

        /*loop forever, ever, even if there is no work queued*/
        boost::asio::io_service::work forever(io_service);

        redisAsyncSetConnectCallback(ac,connectCallback);
        redisAsyncSetDisconnectCallback(ac,disconnectCallback);
        redisAsyncCommand(ac, NULL, NULL, "SET key %b", argv[argc-1], strlen(argv[argc-1]));
        redisAsyncCommand(ac, getCallback, (char*)"end-1", "GET key");

        try
        {
                /*fire up the proactor*/
                io_service.run();
        } catch (std::exception& e) {
                std::cout << "Exception: " << e.what() << "\n";
        }

        return 0;
}

