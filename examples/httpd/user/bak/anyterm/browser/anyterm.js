// browser/anyterm.js
// This file is part of Anyterm; see http://chezphil.org/anyterm/
// (C) 2005 Philip Endecott

// This is version $Name$
//   (if there is no version (e.g. V0-1) mentioned in the previous line,
//    this is probably a snapshot from between "official" releases.)

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.


function min(a,b) {
  if (a<b) return a; else return b;
}

var term;
var kb_buf="";

var send_in_progress=0;
var pending_doc;

var poll_timer;
var min_poll_interval=1000;
var max_poll_interval=10000;
var poll_interval_escalator=1.2;
var poll_interval=min_poll_interval;


function increase_poll_interval() {
  poll_interval=min(poll_interval*poll_interval_escalator,
		    max_poll_interval);
}


function display(c) {
  // Display a character or sequence of characters on the terminal.
  // Will have to do proper terminal emulation, e.g. moving the cursor
  // eventually.  For now, totally naive implementation.
  term.value+=c;
}


function rcv() {
  // Called asynchronously when the pending_doc has returned from the server.
  send_in_progress=0;
  var root=pending_doc.firstChild;
  if (!root) {
    alert("Error: no data");
  }
  if (root.tagName=="error") {
    alert("Error: "+root.firstChild.data);
  } else if (root.tagName=="op") {
    if (root.firstChild) {
      var op = root.firstChild.data;
      display(op);
      poll_interval=min_poll_interval;
    } else {
      increase_poll_interval();
    }
    init_poll();
  } else {
    alert("Unrecognised response: "+root.tagName);
  }
}


function send() {
  send_in_progress=1;
  cancel_poll();
  pending_doc=Sarissa.getDomDocument();
  pending_doc.async = true;
  // pending_doc.method="POST";
  pending_doc.onreadystatechange = load_handler;
  pending_doc.load("foo.anyterm?k="+encodeURIComponent(kb_buf));
  if (kb_buf!="") {
    poll_interval=min_poll_interval;
  }
  kb_buf="";
}


function load_handler() {
  if (pending_doc.readyState==4) {
    rcv();
  }
}


function maybe_send() {
  if (send_in_progress) {
    return;
  }
  send();
}


function process_key(k) {
  kb_buf+=k;
  maybe_send();
}


function init_poll() {
  poll_timer=setTimeout(send,poll_interval);
}


function cancel_poll() {
  clearTimeout(poll_timer);
}


function keypressed(ev) {
  if (!ev) var ev=window.event;
  var kc;
  if (ev.keyCode) kc=ev.keyCode;
  if (ev.which)   kc=ev.which;
if (kc==13) kc=10;
  var k=String.fromCharCode(kc);
  //alert("Key "+k+" pressed");
  process_key(k);
  return false;
}


function initialise() {
  term=document.getElementById("term");
  term.onkeypress=keypressed;
  init_poll();
}


window.onload=initialise;
