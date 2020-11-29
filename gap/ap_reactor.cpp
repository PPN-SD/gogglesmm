/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2021 by Sander Jansen. All Rights Reserved      *
*                               ---                                            *
* This program is free software: you can redistribute it and/or modify         *
* it under the terms of the GNU General Public License as published by         *
* the Free Software Foundation, either version 3 of the License, or            *
* (at your option) any later version.                                          *
*                                                                              *
* This program is distributed in the hope that it will be useful,              *
* but WITHOUT ANY WARRANTY; without even the implied warranty of               *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                *
* GNU General Public License for more details.                                 *
*                                                                              *
* You should have received a copy of the GNU General Public License            *
* along with this program.  If not, see http://www.gnu.org/licenses.           *
********************************************************************************/
#include "ap_defs.h"
#include "ap_reactor.h"

#ifdef _WIN32
#include <windows.h>
#else
#if defined(__linux__)
#define HAVE_PPOLL // On Linux we have ppoll
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <signal.h>
#endif
#include <poll.h>
#include <errno.h>
#endif

namespace ap {

Reactor::Reactor() : pfds(nullptr),nfds(0),mfds(0), timers(nullptr) {
  }

Reactor::~Reactor() {
  freeElms(pfds);

  /// Delete all inputs
  for (FXint i=0;i<inputs.no();i++){
    delete inputs[i];
    }
  inputs.clear();

  /// Delete all deferred
  for (FXint i=0;i<deferred.no();i++){
    delete deferred[i];
    }
  deferred.clear();

  /// Delete all timers
  while(timers) {
    Timer * n = timers->next;
    delete timers;
    timers=n;
    }

  }

#ifdef DEBUG
void Reactor::debug() {
  int ntimers = 0;
  for (Timer * t=timers;t;t=t->next) ntimers++;
  fxmessage("[reactor] timers=%d inputs=%ld deferred=%ld\n",ntimers,inputs.no(),deferred.no());
  }
#endif

void Reactor::dispatch() {
#ifdef _WIN32
  if (result>=WAIT_OBJECT_0 && result<(WAIT_OBJECT_0+nfds)) {
    FXint obj = 0;
    for (FXint i=0;i<inputs.no();i++) {
      if ((inputs[i]->mode&Input::Disabled) || (inputs[i]->mode&(Input::Readable|Input::Writable|Input::Exception))==0) {
        continue;
        }
      if (obj>=(result-WAIT_OBJECT_0) {
        if (WaitForSingleObject(pfds[obj],0)==WAIT_OBJECT_0) {
          inputs[obj]->mode|=Input::IsReadable|Input::Writable;
          inputs[obj]->onSignal();
          }
        }
      obj++;
      }
    }
#else
  FXint i;

  for (i=inputs.no()-1;i>=0;i--) {
    if (pfds[i].revents) {
      inputs[i]->mode|=((pfds[i].revents&POLLIN )          ? Input::IsReadable : 0);
      inputs[i]->mode|=((pfds[i].revents&POLLOUT)          ? Input::IsWritable : 0);
      inputs[i]->mode|=((pfds[i].revents&(POLLERR|POLLHUP))? Input::IsException: 0);
      inputs[i]->onSignal();
      }
    }

  FXint offset=inputs.no();
  for (i=0;i<native.no();i++){
    native[i]->dispatch(pfds+offset);
    offset+=native[i]->no();
    }
#endif
  FXTime now = FXThread::time();
  for (Timer * t = timers;t;t=t->next) {
    if (t->time>0 && t->time<=now) {
      t->time=0;
      t->onExpired();
      if (timers==nullptr) break;
      t = timers; // so onExpired can remove itself from the list...
      }
    }
  }


void Reactor::wait(FXTime timeout) {
#ifdef _WIN32
  result = WaitForMultipleObjects(nfds,pfds,false,(timeout>0) ? timeout / NANOSECONDS_PER_MILLISECOND : FINITE);
#else
  FXint n;
  if (timeout>=0) {
#ifdef HAVE_PPOLL
    struct timespec ts;
    ts.tv_sec  = timeout / NANOSECONDS_PER_SECOND;
    ts.tv_nsec = timeout % NANOSECONDS_PER_SECOND;
#endif
    do {
#ifdef HAVE_PPOLL
      n = ppoll(pfds,nfds,&ts,nullptr);
#else
      n = poll(pfds,nfds,(timeout/NANOSECONDS_PER_MILLISECOND));
#endif
      }
    while(n==-1 && errno==EINTR);
    }
  else {
    do {
#ifdef HAVE_PPOLL
      n = ppoll(pfds,nfds,nullptr,nullptr);
#else
      n = poll(pfds,nfds,-1);
#endif
      }
    while(n==-1 && errno==EINTR);
    }
#endif
  }

FXTime Reactor::prepare() {
  FXTime timeout;
  FXint i;
#ifdef _WIN32
  nfds = inputs.no();
  if (nfds>mfds) {
    mfds=nfds;
    if (pfds==nullptr)
      allocElms(pfds,mfds);
    else
      resizeElms(pfds,mfds);
    }

  for (i=0;i<inputs.no();i++) {
    if ((inputs[i]->mode&Input::Disabled) || (inputs[i]->mode&(Input::Readable|Input::Writable|Input::Exception))==0) {
      continue;
      }
    pfds[i] = inputs[i]->handle;
    inputs[i]->mode&=~(Input::IsReadable|Input::IsWritable|Input::IsException);
    }
#else
  nfds = inputs.no();
  for (i=0;i<native.no();i++) nfds+=native[i]->no();

  if (nfds>mfds) {
    mfds=nfds;
    if (pfds==nullptr)
      allocElms(pfds,mfds);
    else
      resizeElms(pfds,mfds);
    }

  for (i=0;i<inputs.no();i++) {
    if ((inputs[i]->mode&Input::Disabled) || (inputs[i]->mode&(Input::Readable|Input::Writable|Input::Exception))==0) {
      pfds[i].fd       = -inputs[i]->handle;
      pfds[i].events   = 0;
      pfds[i].revents  = 0;
      }
    else {
      pfds[i].fd       = inputs[i]->handle;
      pfds[i].events   = 0;
      pfds[i].events  |= (inputs[i]->mode&Input::Readable)  ? POLLIN : 0;
      pfds[i].events  |= (inputs[i]->mode&Input::Writable)  ? POLLOUT : 0;
      pfds[i].events  |= (inputs[i]->mode&Input::Exception) ? POLLERR|POLLHUP : 0;
      pfds[i].revents  = 0;
      inputs[i]->mode&=~(Input::IsReadable|Input::IsWritable|Input::IsException);
      }
    }

  FXint offset = inputs.no();
  for (i=0;i<native.no();i++) {
    native[i]->prepare(pfds+offset);
    offset+=native[i]->no();
    }
#endif
  FXTime now = FXThread::time();
  Timer * t = timers;
  while(t && t->time<now) t=t->next;
  if (t)
    timeout = t->time - now;
  else
    timeout = -1;

  return timeout;
  }


void Reactor::addNative(Native*n) {
  native.append(n);
  }

void Reactor::removeNative(Native*n){
  native.remove(n);
  }


void Reactor::addInput(Input*w) {
  inputs.append(w);
  }

void Reactor::removeInput(Input*w){
  inputs.remove(w);
  }



void Reactor::addTimer(Timer*t,FXTime time) {
  t->time = time;
  Timer**tt;
  for (tt=&timers;*tt &&((*tt)->time<t->time);tt=&(*tt)->next) {}
  t->next=*tt;
  *tt=t;
  }

void Reactor::removeTimer(Timer*timer) {
  for (Timer**tt=&timers;*tt;tt=&(*tt)->next){
    if ((*tt)==timer){
      Timer * next = (*tt)->next;
      *tt=next;
      break;
      }
    }
  }


void Reactor::addDeferred(Deferred*d) {
  deferred.append(d);
  }

void Reactor::removeDeferred(Deferred*d){
  deferred.remove(d);
  }


FXbool Reactor::dispatchDeferred() {
  FXbool done=false;
  for (FXint i=deferred.no()-1;i>=0;i--){
    if ((deferred[i]->mode&Deferred::Disabled)==0) {
      deferred[i]->run();
      done=true;
      }
    }
  return done;
  }

void Reactor::runPending() {
  if (dispatchDeferred()==false) {
    prepare();
    wait(0);
    dispatch();
    }
  }

void Reactor::runOnce() {
  if (dispatchDeferred()==false) {
    FXTime timeout = prepare();
    wait(timeout);
    dispatch();
    }
  }

void Reactor::runOnce(FXTime wakeup) {
  if (dispatchDeferred()==false) {
    FXTime timeout = prepare();
    if (timeout<0 || wakeup<timeout)
      timeout = wakeup;
    wait(timeout);
    dispatch();
    }
  }



}
