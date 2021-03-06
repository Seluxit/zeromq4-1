/*
    Copyright (c) 2007-2015 Contributors as noted in the AUTHORS file

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

#include "fq.hpp"
#include "pipe.hpp"
#include "err.hpp"
#include "msg.hpp"

zmq::fq_t::fq_t () :
    active (0),
    last_in (NULL),
    current (0),
    more (false)
{
}

zmq::fq_t::~fq_t ()
{
    zmq_assert (pipes.empty ());
}

void zmq::fq_t::attach (pipe_t *pipe_)
{
    pipes.push_back (pipe_);
    pipes.swap (active, pipes.size () - 1);
    active++;
}

void zmq::fq_t::pipe_terminated (pipe_t *pipe_)
{
    const pipes_t::size_type index = pipes.index (pipe_);

    //  Remove the pipe from the list; adjust number of active pipes
    //  accordingly.
    if (index < active) {
        active--;
        pipes.swap (index, active);
        if (current == active)
            current = 0;
    }
    pipes.erase (pipe_);

    if (last_in == pipe_) {
        saved_credential = last_in->get_credential ();
        last_in = NULL;
    }
}

void zmq::fq_t::activated (pipe_t *pipe_)
{
    //  Move the pipe to the list of active pipes.
    pipes.swap (pipes.index (pipe_), active);
    active++;
}

int zmq::fq_t::recv (msg_t *msg_)
{
    return recvpipe (msg_, NULL);
}

int zmq::fq_t::recvpipe (msg_t *msg_, pipe_t **pipe_)
{
    //  Deallocate old content of the message.
    int rc = msg_->close ();
    errno_assert (rc == 0);

    //  Round-robin over the pipes to get the next message.
    while (active > 0) {

        //  Try to fetch new message. If we've already read part of the message
        //  subsequent part should be immediately available.
        bool fetched = pipes [current]->read (msg_);

        //  Note that when message is not fetched, current pipe is deactivated
        //  and replaced by another active pipe. Thus we don't have to increase
        //  the 'current' pointer.
        if (fetched) {
            if (pipe_)
                *pipe_ = pipes [current];
            more = msg_->flags () & msg_t::more? true: false;
            if (!more) {
                last_in = pipes [current];
                current = (current + 1) % active;
            }
            return 0;
        }

        //  Check the atomicity of the message.
        //  If we've already received the first part of the message
        //  we should get the remaining parts without blocking.
        zmq_assert (!more);

        active--;
        pipes.swap (current, active);
        if (current == active)
            current = 0;
    }

    //  No message is available. Initialise the output parameter
    //  to be a 0-byte message.
    rc = msg_->init ();
    errno_assert (rc == 0);
    errno = EAGAIN;
    return -1;
}

bool zmq::fq_t::has_in ()
{
    //  There are subsequent parts of the partly-read message available.
    if (more)
        return true;

    //  Note that messing with current doesn't break the fairness of fair
    //  queueing algorithm. If there are no messages available current will
    //  get back to its original value. Otherwise it'll point to the first
    //  pipe holding messages, skipping only pipes with no messages available.
    while (active > 0) {
        if (pipes [current]->check_read ())
            return true;

        //  Deactivate the pipe.
        active--;
        pipes.swap (current, active);
        if (current == active)
            current = 0;
    }

    return false;
}

zmq::blob_t zmq::fq_t::get_credential () const
{
    return last_in?
        last_in->get_credential (): saved_credential;
}
