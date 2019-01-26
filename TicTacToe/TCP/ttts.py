# File:   ttts.py
# Author: Cassidy Crouse
# Date:   12th Nov 2018
# Desc:   A simple TCP server that plays tictactoe with a client
#         created using the predefined protocol. Currently this
#         server decides its moves randomly and then sends them
#         to the server. Also keeps the master copy of the game
#         board and updates the progress of the game to the user
#         when applicable. Thic TCPServer follows a very simple
#         one byte protocol explained in the code below as well
#         as in tttc.py.

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
		serverSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		serverSocket.bind(('', serverPort))
		serverSocket.listen(1) # for now listen to only one connection

		# gameState keeps track of whether or not the server will continue to
		# listen for messages from the connection that it has opened.
		gameState = -1

		# clientMessage and serverMessage are the last messages sent/received
		# by/from the client/server.
		#
		# The protocol used by the client to the server is as follows:
		# A   - Client would like to begin a new game where the client goes first
		# B   - Client would like to begin a new game where the server goes first
		# 0-8 - Client would like to play the given position on the board
		# ?   - Client would like to know if the game is over yet, server keeps
		#       a master copy of the board and will tell the client if a win,
		#       loss, or draw has occured yet if it receives this message.
		# Q   - Client would like to stop playing, either while a game is in
		#       progress or when a game has been completed.
		#
		# The protocol used by the server to the client is as follows:
		# W   - The server is waiting for the client to make their next move
		# 0-8 - The server is playing the given position on the board
		# S   - The server is telling the client that the server has won
		# C   - The server is telling the client that the client has won
		# D   - The server is telling the client that the game has drawn
		# P   - The server is telling the client that the game is still in progress
		#       this value is ignored by my final implementation but was used for
		#       debugging
		# Q   - The server is telling the client that the connection is being closed
		clientMessage = -1
		serverDecision = -1
		print 'SERVER NOW WAITING FOR CONNECTION'
		# the client will attempt to play at least one game, even if they end it
		# right away.
		gameState = 1
		connectionSocket, addr = serverSocket.accept()
		while gameState > 0:
			clientMessage = connectionSocket.recv(1)
			print 'Client says:', clientMessage

			if clientMessage == 'A':
				newGameA()
				serverDecision = 'W'
				connectionSocket.send('W')
			elif clientMessage == 'B':
				newGameB()
				# make the first move
				serverDecision = randomMove()
				turnCount = turnCount + 1
				connectionSocket.send(serverDecision)
			elif clientMessage == '?':
				# see if the last move was a win for the server or a draw
				serverDecision = checkServer()
				connectionSocket.send(serverDecision)
			elif clientMessage == 'Q':
				gameState = 0
				serverDecision = 'Q'
				connectionSocket.send('Q')
			else:
				# client has played a move that must be validated
				if(valid(clientMessage)):
					accept.remove(clientMessage)
					clientMessage = int(clientMessage)
					gameBoard[clientMessage] = clientSymbol
					turnCount = turnCount + 1

					# see if this move is a win for the client or a draw
					serverDecision = checkClient()
					if serverDecision == 'C':
						# client wins
						connectionSocket.send(serverDecision)
					elif serverDecision == 'D':
						# game is a draw
						connectionSocket.send(serverDecision)
					else:
						# game is not a draw
						serverDecision = randomMove()
						turnCount = turnCount + 1
						connectionSocket.send(serverDecision)
				else:
					# client has played an invalid move and will wait
					# until a valid one is given
					serverDecision = 'W'
					connectionSocket.send(serverDecision)

			print 'Server responds:', serverDecision
			print '	ACCEPT DUMP:', accept
			print '	GAME DUMP:', gameBoard

		# user has decided that they would like to close the connection
		connectionSocket.send('Q')
		connectionSocket.close()
		serverSocket.close()
		print 'CONNECTION CLOSED DUE TO GAME END'
		sys.exit(0)
	except KeyboardInterrupt:
		# tell the client the server has ended the game, only bother
		# attempting this if there is actually a connection
		connectionSocket.send('Q')
		connectionSocket.close()
		serverSocket.close()
		print 'CONNECTION CLOSED MANUALLY AT SERVER'
		sys.exit(0)
	
		
	






