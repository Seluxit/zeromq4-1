/*
    Copyright (c) 2007-2016 Contributors as noted in the AUTHORS file

    This file is part of libzmq, the ZeroMQ core engine in C++.

    libzmq is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License (LGPL) as published
    by the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    As a special exception, the Contributors give you permission to link
    this library with independent modules to produce an executable,
    regardless of the license terms of these independent modules, and to
    copy and distribute the resulting executable under terms of your choice,
    provided that you also meet, for each linked independent module, the
    terms and conditions of the license of that module. An independent
    module is a module which is not derived from or based on this library.
    If you modify this library, you must extend this exception to your
    version of the library.

    libzmq is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "testutil.hpp"

int main(int, char**) {
    void* ctx = zmq_ctx_new();
    assert(ctx != NULL);

    void* pub = zmq_socket(ctx, ZMQ_PUB);
    assert(pub != NULL);

    int rc = zmq_bind(pub, "tcp://*:23456");
    assert(rc == 0);

    void* sub = zmq_socket(ctx, ZMQ_SUB);
    assert(sub != NULL);

    rc = zmq_setsockopt(sub, ZMQ_SUBSCRIBE, "", 0);
    assert(rc == 0);

    const char* endpoint_sub = "tcp://localhost:23456";
    rc = zmq_connect(sub, endpoint_sub);
    assert(rc == 0);

    msleep(100);

    rc = s_sendmore(pub, "data in first frame");
    assert(rc >= 0);

    rc = s_send(pub, "data in second frame");
    assert(rc >= 0);

    char* message = s_recv(sub);
    assert(message);
    printf("<%s>\n", message);
    free(message);

    rc = zmq_disconnect(sub, endpoint_sub);
    assert(rc == 0);
    printf("disconnected SUB socket\n");

    printf("receiving part 2 of message: ");
    /* => Assertion failed: !more (src/fq.cpp:117) */
    message = s_recv(sub);
    assert(message);
    printf("<%s>\n", message);
    free(message);

    rc = zmq_close (sub);
    assert (rc == 0);

    rc = zmq_close (pub);
    assert (rc == 0);



    void* rep = zmq_socket(ctx, ZMQ_ROUTER);
    assert(rep != NULL);

    rc = zmq_bind(rep, "tcp://*:234567");
    assert(rc == 0);

    void* req = zmq_socket(ctx, ZMQ_REQ);
    assert(req != NULL);

    const char* endpoint_req = "tcp://localhost:234567";
    rc = zmq_connect(req, endpoint_req);
    assert(rc == 0);

    msleep(100);

    rc = s_sendmore(req, "data in first frame");
    assert(rc >= 0);

    rc = s_send(req, "data in second frame");
    assert(rc >= 0);

    message = s_recv(rep);
    assert(message);
    printf("<%s>\n", message);
    free(message);

    rc = zmq_disconnect(req, endpoint_req);
    assert(rc == 0);
    printf("disconnected REQ socket\n");

    printf("receiving part 2 of message: ");
    /* => Assertion failed: !more (src/fq.cpp:117) */
    message = s_recv(rep);
    assert(message);
    printf("<%s>\n", message);
    free(message);

    rc = zmq_close (rep);
    assert (rc == 0);

    rc = zmq_close (req);
    assert (rc == 0);


    rc = zmq_ctx_shutdown (ctx);
    assert (rc == 0);

    return 0;
}
