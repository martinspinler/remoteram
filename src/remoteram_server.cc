#include <chrono>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>

#include <protobuf/remoteram.grpc.pb.h>

#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include "remoteram.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;

using remoteram::RemoteRam;
using remoteram::ram_req;
using remoteram::ram_resp;

class RemoteRamServerImpl final : public RemoteRam::Service {
	public:
		explicit RemoteRamServerImpl(const std::string& path) : m_verbosity(0) {}
		Status Ram_req(ServerContext* context, const ram_req *req, ram_resp *resp) override {
			if (req->type() == 0) {
				char * data = (char*) req->addr();
				if (m_verbosity)
					printf("DMA write request %08x %04x\n", req->addr(), req->nbyte());
				resp->set_status(0);
				memcpy(data, req->data().c_str(), req->nbyte());

			} else {
				if (m_verbosity)
					printf("DMA read request <%08x %04x\n", req->addr(), req->nbyte());
				const char * data = (const char*) req->addr();
				resp->set_status(0);
	                        resp->set_data(std::string((const char*)data, req->nbyte()));
			}
			return Status::OK;
		}
		int m_verbosity;
};


extern "C" {

struct __remoteram {
	Server* server;
	RemoteRamServerImpl * service;
};

remoteram_t remoteram_create_server(const char *addr, int verbosity)
{
	remoteram_t rr = new struct __remoteram;

	std::string server_address(addr);
	rr->service = new RemoteRamServerImpl("");
	rr->service->m_verbosity = verbosity;

	ServerBuilder builder;
	builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
	builder.RegisterService(rr->service);
	rr->server = builder.BuildAndStart().release();

	if (verbosity)
		std::cout << "Server listening on " << server_address << std::endl;

	return rr;
}

void remoteram_destroy_server(remoteram_t rr)
{
	rr->server->Shutdown();
	rr->server->Wait();
	delete rr->service;
}

}
