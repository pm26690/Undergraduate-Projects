(For a readme of the code contained in this sub-repository, see the actual code
in tttc.py and ttts.py)

Protocol Overview:

This protocol is designed for communications between a tic-tac-toe server
and a tic-tac-toe client which will be playing on the game board hosted at
the server. The server keeps the master copy of the board, so if the client
decides to alter their own board it would be out of synchronization with that
of the server and would not affect the master copy in any way. The server
ultimately decides who wins and who loses, and may or may not send a proper
message to notify client(s) that the server is closing if and when the server
does close with client(s) in progress.

Reliability:

All of the messages that will be sent between the client and server will be
done with UDP sockets. As such, reliability must be built into the client in
order to handle dropped datagrams and datagrams with bit errors and other
anomalies.

The type of reliability that shall be built into the client and server is best
described as a NAK-less protocol. Instead of sending negative acknowledgements
the client and server shall wait for the timeout at the other end to occur
and then wait for the datagram to be automatically resent. The specifics of
the NAK-less protocol are as follows:

* The client and server shall both have a timeout of 1000 miliseconds. If the
  acknowledgement for a datagram is not received within this window, the
  datagram will be automatically resent.
* The client and server shall adhere to the messages sent in the protocol. If
  a message is received that is not in the protocol, it is assumed to be a
  corrupt datagram, and a forced timeout wait shall occur.
* The client and server have expected messages at any state of the game. If a
  message is received that is not expected at that state of the game, it is
  assumed to be a corrupt or repeat datagram, and a forced timeout wait shall
  occur.
* With the last specification in mind, if the client or server receive an
  acknowledgement for a message that they did not send, they should resend
  their last message. This is illustrated in the following way:
  1. Server asks client to make a move
  2. Client receives this message and acknowledges it with an appropriate, valid move.
  3. Server receives the move, but the move is corrupted, the server acknowledges
     the corrupted (but still valid) move.
  4. Client receives acknowledgement for the move that they did not make. Send the move
     again.
* With the last specification in mind, if the client or server receive a message
  that they do expect at that state of the game, they shall send an acknowledge-
  ment for that message. The acknowledgements for messages are specified in the
  server and client messages section. In general, the acknowledgement for a message
  is sending that same message in return.

Expected Order:
* The order in which messages are sent and receive shall adhere to the
  the following specifications.

* Beginning the game:
  1. Client shall send a "A" or "B" to the server indicating they would like to begin
     a game. (A if the client goes first, B if the server goes first).
  2. Server shall acknowledge that they received this message by sending the same message
     back to the client (essentially a handshake).

  * If the client goes first ("A" handshake):
    3a. Upon receiving acknowledgement from the server to begin (A), the client shall send
        appropriate move (0-8).
    4a. The server shall acknowledge this by sending the same move back.
        * if the client does not accept this acknowledgement it shall repeat 3a.
    5a. The client shall acknowledge this by sending the end of turn message "E".
        * if the server does not acknowledge this by sending a move back, client shall
          resend "E"

  * If the server goes first ("B" handshake):
    3b. Upon receiving acknowledgement from the server to begin (B), the client shall
        send the server the end of turn message "E" (even though it is not the end of
        anyone's turn).
    4b. The server shall acknowledge this by sending their move to the client.
        * if the client does not receive this acknowledgement, it shall repeat 3b.
    5b. The client shall acknowledge this by sending the same move back.
        * if the server does not receive this acknowledgement, it shall repeat 4b.
    6b. The server shall acknowledge this by sending the end of turn message "E".
        * if the client does not accept this acknowledgement (by moving on to the
          specifications defined below), it shall repeat 5b.

* Server makes a move (non-first turn):
  1. It is the servers turn whenever the client sends the "E" end of turn message. If the
     server receives this message it shall immediately send a move (or game status) as an
     acknowledgement.
	* Appropriate moves sent by the server are 0-8
	* Client win is denoted by "C", draw is denoted by "D"

  * If the game is not over when server starts its turn, server makes a move:
    2a. The server shall acknowledge the client's "E" message by sending a move
        * if the client does not acknowledge this by sending the move back, server
          waits for client to resend "E"
    3a. The client shall acknowledge the move by sending the move back.
        * if the server does not accept this acknowledgement, it shall repeat 2a.
    4a. The server shall acknowledge this by sending the end of turn message "E",
        alternatively if the game is over with this move, it will send "S" if it
        won or "D" if the game is a draw.

  * If the game is over when server starts its turn, server sends game status:
    2b. The server shall acknowledge the client's "E" message by sending game status,
        the game status is "C" if the client won the game, or "D" if there was a draw.
        * if the client does not acknowledge this by sending the status back, the
          server waits for client to resend "E"
    3b. The client shall acknowledge the game status by sending it back.
        * if the server does not acknowledge this with "Q", repeat 2b.
    4b. The server shall acknowledge this by ending the game with "Q".

* Client makes a move (non-first turn):
   1. It is the client's turn whenever the server sends the "E" end of turn message. If the
      client receives this message it shall send its next appropriate move. There is still
      a timeout associated with this message, if the server has yet to receive a move, it will
      continue sending "E". If the move sent by the client is not appropriate, resend "E".
      * Appropriate moves sent by the client are 0-8
      * Server must validate moves made by client, if a move is invalid server shall send
        "W", as opposed to "E".

   * If the move made by the client is not valid:
     2a. The server shall acknowledge that the move made by the client is invalid by sending
         back "I", which symbolizes that the move was invalid.
         * if the client does not acknowledge this by sending "I", resend "I".
     3a. The server shall acknowledge the client's "I" by sending "W".
         * if the client does not receive "W", resend "I"
     4a. The client shall acknowledge "W" by sending an appropriate move
         * if the move sent by the client is not valid, repeat 2a

   * If the move made by the client is valid:
     2b. The server shall acknowledge the appropriate, valid move by sending it back.
         * if the client does not accept this acknowledgement, the server will wait for the
           client to resend their move.
     3b. The client shall acknowledge this by ending its turn with "E".
         * if the server does not acknowledge this by sending a move (or game status) back
           to the client, resend "E".

Messages accepted (expected) by server:

A: client requests to start game, client goes first
B: client requests to start game, server goes first
S: server win message confirmation (acknowledgement)
C: client win message confirmation (acknowledgement)
D: draw message confirmation (acknowledgement)
I: invalid move (acknowledgement)
E: end of client turn message
0-8: appropriate move / move confirmation (acknowledgement)

Messages that shall be accepted (expected) by the client:

A: server confirms start of game, client goes first (acknowledgement)
B: server confirms start of game, server goes first (acknowledgement)
S: server win message
C: client win message
I: invalid move notice
W: server waiting for appropriate move (see expected order section for more info)
D: draw message
E: end of server turn message
0-8: appropriate move / move confirmation (acknowledgement)

* All other messages shall be ignored absolutely, all messages received out of order (see
  expected order section) shall be ignored absolutely.

