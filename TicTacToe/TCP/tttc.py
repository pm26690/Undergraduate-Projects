# File:   tttc.py
# Author: Cassidy Crouse
# Date:   12th Nov 2018
# Desc:   A simple TCP client used to play tic tac toe with the server
#         located at the IP provided on execution. This TCPClient follows
#         a very simple one byte protocol explained in the code below near
#		  the main function as well as in ttts.py

import socket
import sys

#
# Print the tictactoe board in a human readable format so the user knows
# what it currently looks like and can make their decision.
#
def printBoard():
	#
	# my tictactoe board has the following layout (indexing from zero):
	#
	#       0 | 1 | 2
	#      ---+---+---
	#       3 | 4 | 5
	#      ---+---+---
	#       6 | 7 | 8
	#
	# the board will be filled out in the following way:
	# 
	# 	  ' ': space character; a space for valid moves
	#      'X': the symbol of the player who went first
	#      'O': the symbol of the player who went second
	#
	# for example the following board:
	#  
	# 	  gameBoard = ['X','O',' ',' ','X',' ',' ','O','X']
	#
	#                      0 | 1 | 2        X | O |  
	#                     ---+---+---      ---+---+---
	#      gameBoard[n] =  3 | 4 | 5  ===>    | X | 
	#                     ---+---+---      ---+---+---
	#                      6 | 7 | 8          | O | X
	#
	# is a board in which 'X' has just won with their third move.
	#

	if gameBoard != -1:
		print ' '
		print 'Moves Made: ', turnCount, ' You are Symbol: ', clientSymbol
		print gameBoard[0], '|', gameBoard[1], '|', gameBoard[2]
		print '--+---+--'
		print gameBoard[3], '|', gameBoard[4], '|', gameBoard[5]
		print '--+---+--'
		print gameBoard[6], '|', gameBoard[7], '|', gameBoard[8]
		print ' '
	else:
		print 'Cannot print a board which is not yet defined.'

#
# Start a new game, player must choose their symbol and other variables
# must be defined correctly
#
def newGame(type):
	# need to modify global variables
	global clientSymbol
	global serverSymbol
	global gameBoard
	global turnCount
	global accept

	if type == 1:
		clientSymbol = 'X'
		serverSymbol = 'O'
	else:
		clientSymbol = 'O'
		serverSymbol = 'X'
	
	# remind the player what the board looks like and how to play
	print ' '
	print 'DIRECTIONS:'
	print 'When prompted to make a move, keep in mind the number that corresponds to each position:'
	print ' '
	print '0 | 1 | 2'
	print '--+---+--'
	print '3 | 4 | 5'
	print '--+---+--'
	print '6 | 7 | 8'
	print ' '
	print 'If you enter an invalid position, you will be prompted again.'
	print 'If you would like to quit at any point in time, enter \'Q\' instead.'
	print ' '

	# need a fresh board and turn counter to be 0
	accept = ['0', '1', '2', '3', '4', '5', '6', '7', '8', 'Q']
	gameBoard = [' ',' ',' ',' ',' ',' ',' ',' ',' ']
	turnCount = 0			

#
# program expects only one argument, the destination IP for the server.
# it doesn't particularly care if more arguments are provided as it just
# doesn't do anything with them.
#
if __name__ == '__main__':
	# need to be able to modify global variables
	global serverName
	global serverPort
	global clientSocket
	global gameBoard
	global turnCount
	global accept

	# gameType controls whether or not the player goes first
	gameType = 0

	# check the arguments and flags
	if len(sys.argv) < 3:
		print 'Invalid Arguments!'
		print 'Use at least -s flag followed by IP'
		print 'For example provide \'-s <SERVER_IP>\''
		sys.exit()
		
	elif len(sys.argv) > 4:
		print 'Invalid Arguments!'
		print 'For example provide \'-c -s <SERVER_IP>\''
		sys.exit()
	else:
		if len(sys.argv) == 3:
			if sys.argv[1] == '-s':
				gameType = 2
				serverName = sys.argv[2]
			else:
				print 'Invalid Arguments!'
				print 'Missing server identifier flag.'
				print 'For example provide \'-s <SERVER_IP>\''
				sys.exit()
		else:
			if sys.argv[1] == '-c':
				if sys.argv[2] == '-s':
					gameType = 1
					serverName = sys.argv[3]
				else:	
					print 'Invalid Arguments!'
					print 'Missing server identifier flag.'
					print 'For example provide \'-c -s <SERVER_IP>\''
					sys.exit()
			else:
				print 'Invalid Arguments!'
				print 'Mismatched flags, -c expected first with 3 arguments.'
				print 'For example provide \'-c -s <SERVER_IP>\''
				sys.exit()

	# server port is fixed at port 13037
	serverPort = 13037

	print 'IP is: ' , serverName
	print 'Port is: ' , serverPort

	# if the destination IP is valid, the four tuple is now complete so
	# attempt to make a connection with the server.
	try:
		clientSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		clientSocket.connect((serverName, serverPort))
	except socket.error:
		print 'Server at ', serverName, ' refused connection, perhaps no such server exists'
		print 'or perhaps the IP provided is invalid.'
		print 'Suggest checking the status of the server and the IP and trying again.'
		# no destination server present, exit the program
		sys.exit()

	# gameState controls whether or not a game is to be played, because
	# the program was run we attempt to play at least one game so this
	# starts at 1.
	gameState = 1

	# clientMessage and serverMessage are the last messages sent/received
	# by/from the client/server. All messages sent by the client should be
	# validated rigorously to meet the following specifications at the level
	# of the client and not by the server. The only error message returned
	# by the server is 'W' which specifies that the server is waiting for
	# the client to make a valid move, which is to say a move that is not
	# already played on the board.
	#
	# The protocol used by the client to the server is as follows:
	# A   - Client would like to begin a new game where the client goes first
	# B   - Client would like to begin a new game where the server goes first
	# 0-8 - Client would like to play the given position on the board
	# ?   - Client would like to know if the game is over yet, server keeps
	#		a master copy of the board and will tell the client if a win,
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
	serverMessage = -1

	turnCount = 0
	
	try:
		# this keeps track of the tictactoe game
		while gameState > 0:
			if turnCount == 0:
				# handle special case of first turn
				newGame(gameType)
				printBoard()
				if gameType == 1:
					clientSocket.send('A')
					clientSocket.recv(1) # server signals waiting after seing 'A'

					# Player safely makes first move, can't be an invalid move
					clientDecision = raw_input('Play an empty pos (0-8) or Q to quit: ')
					while clientDecision not in accept:
						clientDecision = raw_input('Play an empty pos (0-8) or Q to quit: ')
					# if the client would like to do so, end the game
					if clientDecision == 'Q':
						gameState = 0
					else:
						clientSocket.send(clientDecision)
						turnCount = turnCount + 1
				else:
					# AI makes first move
					clientSocket.send('B')
					turnCount = turnCount + 1
			else:
				# get the AI move and let player make their decision if no game
				# over conditions are met
				serverMessage = clientSocket.recv(1)
			
				if serverMessage == 'W':
					# client sent invalid position, server is waiting for valid move
					clientDecision = raw_input('Play an empty pos (0-8) or Q to quit: ')
					while clientDecision not in accept:
						clientDecision = raw_input('Play an empty pos (0-8) or Q to quit: ')
					clientSocket.send(clientDecision)
				elif serverMessage == 'C':
					decision = int(clientDecision)
					gameBoard[decision] = clientSymbol
					turnCount = turnCount + 1
					printBoard()
					print ' '
					print 'Notice: Congratulations you have beaten the server!'
					gameState = 0
				elif serverMessage == 'D':
					print ' '
					print 'Notice: The game is a draw.'
					gameState = 0
				elif serverMessage == 'Q':
					print ' '
					print 'Notice: server is closing connection, game has ended.'
					sys.exit()
				else:
					if gameType == 1:				
						# clients position was valid, display the new board
						decision = int(clientDecision)
						gameBoard[decision] = clientSymbol
						turnCount = turnCount + 1
						printBoard()
					elif turnCount > 1:
						decision = int(clientDecision)
						gameBoard[decision] = clientSymbol
						turnCount = turnCount + 1
						printBoard()
					
					# display the servers move
					decision = int(serverMessage)
					gameBoard[decision] = serverSymbol
					turnCount = turnCount + 1
					print ' '
					print 'Notice: Server has played position ', serverMessage
					printBoard()

					# ask the server if their move was a win
					clientSocket.send('?')
					serverMessage = clientSocket.recv(1)
					if serverMessage == 'S':
						print ' '
						print 'Notice: Unfortunately the server has beaten you.'
						gameState = 0
					elif serverMessage == 'D':
						print ' '
						print 'Notice: The game is a draw.'
						gameState = 0
					else:
						# get players next move and send it
						clientDecision = raw_input('Play an empty pos (0-8) or Q to quit: ')
						while clientDecision not in accept:
							clientDecision = raw_input('Play an empty pos (0-8) or Q to quit: ')
						clientSocket.send(clientDecision)
						# if the client would like to do so, end the game
						if clientDecision == 'Q':
							gameState = 0
		# END WHILE LOOP

		# the game has ended
		clientSocket.send('Q')
		clientSocket.recv(1)
		clientSocket.close()
	except KeyboardInterrupt:
		# keyboard interrupt quits the game a little more violently
		clientSocket.send('Q')
		clientSocket.recv(1)
		clientSocket.close()
		sys.exit()

