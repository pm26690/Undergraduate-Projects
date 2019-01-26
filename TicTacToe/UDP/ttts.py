# File:   ttts.py
# Author: Cassidy Crouse
# Date:   10th Dec 2018
# Desc:   A simple UDP server that plays tictactoe with a client
#         created using the protocol defined in README.txt. this
#         server makes random moves, and accepts only the inputs
#         defined in the protocol in the order they are defined in
#         the protocol. Currently the server plays only a single
#         game with a single client and then closes the game. If
#         at any point the client is closed before the server, the
#         server will continue sending messages to a non-existent
#         client. If any other client than the one that initially
#         connected sends the server a move, the server will treat
#         that client as if it was the initial client.

import socket
import sys
import random

#
# Start a new game, client has decided to go first
#
def newGameA():
	global clientSymbol
	global serverSymbol
	global accept
	global gameBoard
	global turnCount

	clientSymbol = 'X'
	serverSymbol = 'O'
	# need a fresh board and turn counter to be 0
	accept = ['0', '1', '2', '3', '4', '5', '6', '7', '8']
	gameBoard = [' ',' ',' ',' ',' ',' ',' ',' ',' ']
	turnCount = 0

#
# Start a new game, client has decided to go second
#
def newGameB():
	global clientSymbol
	global serverSymbol
	global accept
	global gameBoard
	global turnCount

	clientSymbol = 'O'
	serverSymbol = 'X'
	# need a fresh board and turn counter to be 0
	accept = ['0', '1', '2', '3', '4', '5', '6', '7', '8']
	gameBoard = [' ',' ',' ',' ',' ',' ',' ',' ',' ']
	turnCount = 0

#
# Check if the server has won or the game has drawn
#
def checkServer():
	if gameBoard[0] == serverSymbol and gameBoard[1] == serverSymbol and gameBoard[2] == serverSymbol:
		return 'S'
	elif gameBoard[3] == serverSymbol and gameBoard[4] == serverSymbol and gameBoard[5] == serverSymbol:
		return 'S'
	elif gameBoard[6] == serverSymbol and gameBoard[7] == serverSymbol and gameBoard[8] == serverSymbol:
		return 'S'
	elif gameBoard[0] == serverSymbol and gameBoard[3] == serverSymbol and gameBoard[6] == serverSymbol:
		return 'S'
	elif gameBoard[1] == serverSymbol and gameBoard[4] == serverSymbol and gameBoard[7] == serverSymbol:
		return 'S'
	elif gameBoard[2] == serverSymbol and gameBoard[5] == serverSymbol and gameBoard[8] == serverSymbol:
		return 'S'
	elif gameBoard[0] == serverSymbol and gameBoard[4] == serverSymbol and gameBoard[8] == serverSymbol:
		return 'S'
	elif gameBoard[6] == serverSymbol and gameBoard[4] == serverSymbol and gameBoard[2] == serverSymbol:
		return 'S'
	elif turnCount == 9:
		return 'D'
	else:
		return 'P'
#
# Check if the client has won or the game has drawn
#
def checkClient():
	if gameBoard[0] == clientSymbol and gameBoard[1] == clientSymbol and gameBoard[2] == clientSymbol:
		return 'C'
	elif gameBoard[3] == clientSymbol and gameBoard[4] == clientSymbol and gameBoard[5] == clientSymbol:
		return 'C'
	elif gameBoard[6] == clientSymbol and gameBoard[7] == clientSymbol and gameBoard[8] == clientSymbol:
		return 'C'
	elif gameBoard[0] == clientSymbol and gameBoard[3] == clientSymbol and gameBoard[6] == clientSymbol:
		return 'C'
	elif gameBoard[1] == clientSymbol and gameBoard[4] == clientSymbol and gameBoard[7] == clientSymbol:
		return 'C'
	elif gameBoard[2] == clientSymbol and gameBoard[5] == clientSymbol and gameBoard[8] == clientSymbol:
		return 'C'
	elif gameBoard[0] == clientSymbol and gameBoard[4] == clientSymbol and gameBoard[8] == clientSymbol:
		return 'C'
	elif gameBoard[6] == clientSymbol and gameBoard[4] == clientSymbol and gameBoard[2] == clientSymbol:
		return 'C'
	elif turnCount == 9:
		return 'D'
	else:
		return 'P'
	
#
# Server picks a random position from the available valid positions
# and plays it
#
def randomMove():
	global accept
	global gameBoard

	decision = random.choice(accept)
	accept.remove(decision)
	decisionNum = int(decision)
	gameBoard[decisionNum] = serverSymbol
	return decision

#
# Server validates the given player move
#
def valid(move):
	if move in accept:
		return True
	else:
		return False

#
# Opens the server socket which will listen for messages from
# a tictactoe client, the server script accepts no parameters.
#
if __name__ == '__main__':
	# need to be able to modify global variables
	global serverPort
	global serverSocket
	global gameBoard
	global clientSymbol
	global serverSymbol
	global turnCount
	global accept

	try:
		serverPort = 13037
		serverSocket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
		serverSocket.bind(('', serverPort))
		serverSocket.settimeout(1)

		# gameState keeps track of whether or not the server will continue to
		# listen for messages from the connection that it has opened.
		gameState = -1

		# clientMessage and serverMessage are the last messages sent/received
		# by/from the client/server.
		#
		# serverDecision and clientDecision ar ethe last moves made by the server/
		# client
		
		clientMessage = -1
		serverMessage = -1

		serverDecision = -1
		serverLastMove = -1

		clientDecision = -1
		clientLastMove = -1

		isServersTurn = 0
		serverHasSent = 0

		print 'SERVER NOW WAITING FOR CONNECTION'

		clientAddress = -1
		clientPort = -1
		helloMessage = -1
		accepted = 0

		# server waits indefinitely for a connection
		while accepted == 0:
			try:
				helloMessage, twoTuple = serverSocket.recvfrom(1)
				print helloMessage, ' received from ', twoTuple
				clientAddress = twoTuple[0]
				clientPort = twoTuple[1]
			
				if helloMessage == 'A':
					serverMessage = 'A'
					clientMessage = 'A'
					isServersTurn = 0
					newGameA()
					serverSocket.sendto(serverMessage, (clientAddress, clientPort))
					accepted = 1
				elif helloMessage == 'B':
					serverMessage = 'B'
					clientMessage = 'B'
					isServersTurn = 1
					newGameB()
					serverSocket.sendto(serverMessage, (clientAddress, clientPort))
					accepted = 1
				else:
					print 'client gave bad helloMessage, waiting for resend'
			except socket.timeout:
				print 'SERVER STILL WAITING FOR CONNECTION'

		gameState = 1
		while gameState > 0:
			# server is waiting for a message from the client, if it does
			# not receive the message it expects, it will automatically resend
			# the last message it sent to the client, if a timeout occurs, it
			# will resend the last message it sent to the client
			try: 
				newMessage, twoTuple = serverSocket.recvfrom(1)

				# debug print only on different message
				if newMessage != clientMessage:
					print 'client says: ', newMessage
					print '	last client message: ', clientMessage
					print '	last server message: ', serverMessage
					print '	turn count: ', turnCount
					print '	game dump: ', gameBoard
					print '	accept dump: ', accept

				serverHasSent = 0
				
				if newMessage.isdigit():
					newMessageInt = int(newMessage)

				# automatic resend confirmation A
				if newMessage == 'A' and serverMessage == 'A':
					clientMessage = newMessage
					serverMessage = newMessage
					serverHasSent = 1
					serverSocket.sendto(serverMessage, (clientAddress, clientPort))

				# automatic resend confirmation B
				if newMessage == 'B' and serverMessage == 'B' and serverHasSent == 0:
					clientMessage = newMessage
					serverMessage = newMessage
					serverHasSent = 1
					serverSocket.sendto(serverMessage, (clientAddress, clientPort))

				# client has acknowledged that it sent an invalid move, send W to request
				# new input from the user
				if newMessage == 'I':
					clientMessage = newMessage
					serverMessage = 'W'
					serverHasSent = 1
					serverSocket.sendto(serverMessage, (clientAddress, clientPort))

				# client wants server to make a move (or give status)
				if newMessage == 'E' and serverHasSent == 0:
					if(turnCount == 0 and isServersTurn == 1):
						# is this a duplicate E?
						if clientMessage == 'B':
							# this is not a resend E on the first turn, play a random
							# move on the board
							serverDecision = randomMove()
							serverLastMove = int(serverDecision)
							gameBoard[serverLastMove] = serverSymbol
							turnCount = turnCount + 1
							isServersTurn = 1

							clientMessage = newMessage
							serverMessage = serverDecision
							serverHasSent = 1
							serverSocket.sendto(serverMessage, (clientAddress, clientPort))
						else:
							# automatic resend last move played by server
							clientMessage = newMessage
							serverHasSent = 1
							serverSocket.sendto(serverMessage, (clientAddress, clientPort))
					else:
						# is this a duplicate E?
						if clientMessage == 'E':
							# automatic resend last move made by server
							clientMessage = newMessage
							serverHasSent = 1
							serverSocket.sendto(serverMessage, (clientAddress, clientPort))
						else:
							# not a duplicate E update the board and check status
							gameBoard[clientLastMove] = clientSymbol
							accept.remove(clientDecision)
							turnCount = turnCount + 1
							isServersTurn = 1						

							temp = checkClient()

							if(temp == 'P'):
								# game still in progress, choose a random move and play it
								serverDecision = randomMove()
								serverLastMove = int(serverDecision)
								gameBoard[serverLastMove] = serverSymbol
								turnCount = turnCount + 1

								clientMessage = newMessage
								serverMessage = serverDecision
								serverHasSent = 1
								serverSocket.sendto(serverMessage, (clientAddress, clientPort))
							if(temp == 'C'):
								# client win
								clientMessage = newMessage
								serverMessage = 'C'
								serverHasSent = 1
								gameState = 0
								serverSocket.sendto(serverMessage, (clientAddress, clientPort))
							if(temp == 'D'):
								# game draw
								clientMessage = newMessage
								serverMessage = 'D'
								serverHasSent = 1
								gameState = 0
								serverSocket.sendto(serverMessage, (clientAddress, clientPort))
				# client sending a number
				if newMessage.isdigit() and serverHasSent == 0:
					if isServersTurn == 0:
						# client attempting to make a move, server must validate it
						if newMessage in accept:
							# move is valid, send it back to the client as confirmation
							# (also functions as automatic resend on duplicate numbers)
							clientDecision = newMessage
							clientLastMove = newMessageInt
							clientMessage = newMessage
							serverMessage = newMessage
							serverHasSent = 1
							serverSocket.sendto(serverMessage, (clientAddress, clientPort))
						else:
							# move is invalid, send back 'I', this would tell the client
							# that the move they sent is invalid
							clientMessage = newMessage
							serverMessage = 'I'
							serverHasSent = 1
							serverSocket.sendto(serverMessage, (clientAddress, clientPort))
					else:
						# client confirming server move
						if newMessage == serverMessage:
							print 'client confirmed move: ', serverDecision
							# server move confirmed, check status, end turn if we havent
							# won or drawn
							temp = checkServer()
							
							if(temp == 'P'):
								# game still in progress, end turn
								isServersTurn = 0
								serverMessage = 'E'
								clientMessage = newMessage
								serverHasSent = 1
								serverSocket.sendto(serverMessage, (clientAddress, clientPort))
							if(temp == 'S'):
								# server win
								serverMessage = 'S'
								clientMessage = newMessage
								serverHasSent = 1
								gameState = 0
								serverSocket.sendto(serverMessage, (clientAddress, clientPort))
							if(temp == 'D'):
								# draw game
								serverMessage = 'D'
								clientMessage = newMessage
								serverHasSent = 1
								gameState = 0
								serverSocket.sendto(serverMessage, (clientAddress, clientPort))
			except socket.timeout:
				# if we timeout we send the same message back as before
				print 'client said: ', clientMessage
				print 'server responds: ', serverMessage
				serverSocket.sendto(serverMessage, (clientAddress, clientPort))
		# END WHILE LOOP

		# the game has ended, close client gracefully
		clientClosed = 0		
		
		# send the client the game outcome until they confirm it, at which point
		# we send the Q message which means the client is closing the game (don't
		# bother making sure that the client sees it because they've already confirmed
		# the game is over)
		while clientClosed == 0:
			try:
				newMessage, twoTuple = serverSocket.recvfrom(1)
				print 'client says: ', newMessage
				if newMessage == serverMessage:
					clientClosed = 1
					serverMessage = 'Q'
					clientMessage = newMessage
					serverSocket.sendto(serverMessage, (clientAddress, clientPort))
				else:
					clientMessage = newMessage
					serverSocket.sendto(serverMessage, (clientAddress, clientPort))
				print 'server responds: ', serverMessage
			except socket.timeout:
				serverSocket.sendto(serverMessage, (clientAddress, clientPort))
				print 'server responds: ', serverMessage
		
		serverSocket.close()
		print 'CONNECTION CLOSED DUE TO GAME END'
		sys.exit(0)
	except KeyboardInterrupt:
		# tell the client the server has ended the game, only bother
		# attempting this if there is actually a connection
		#connectionSocket.send('Q')
		#connectionSocket.close()
		serverSocket.close()
		print 'CONNECTION CLOSED MANUALLY AT SERVER'
		sys.exit(0)
