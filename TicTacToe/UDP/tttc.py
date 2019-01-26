# File:   tttc.py
# Author: Cassidy Crouse
# Date:   10th Dec 2018
# Desc:   A simple UDP client used to play tic tac toe with the server
#         located at the IP provided on execution. Follows a simple one byte
#         NAK-less protocol explained in the README.txt. I had a hard time
#         trying to test my client on high traffic networks to see if it was
#         actually reliable, but if my logic is correct at the server, the
#         server should at least have reliability.

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
		print 'You are Symbol: ', clientSymbol
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

#
# program expects two arguments, game configuration flag -s or -c, and the server ip
#
if __name__ == '__main__':
	# need to be able to modify global variables
	global serverName
	global serverPort
	global clientSocket
	global gameBoard
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

	# variables used to control the flow of the program

	clientMessage = -1
	serverMessage = -1
		
	clientDecision = -1
	clientLastMove = -1
	serverDecision = -1
	serverLastMove = -1
	twoTuple = -1

	isClientsTurn = 0
	turnCount = 0
	gameState = 0

	# if the destination IP is valid, the four tuple is now complete but
	# UDP is connectionless so can't connect like with TCP. Instead we send
	# the server a beginning of game message. If it is acknowledged, the game
	# will begin. If it is not after several timeouts, we stop making attempts
	# and assume the server does not exist.
	try:
		clientSocket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
		clientSocket.settimeout(1) # timeout of 1 second

		attempts = 0
		accepted = 0

		if gameType == 1:
			clientMessage = 'A'
		else:
			clientMessage = 'B'

		clientSocket.sendto(clientMessage, (serverName, serverPort))

		while attempts < 5 and accepted == 0:
			try:
				serverMessage, twoTuple = clientSocket.recvfrom(1)
				if serverMessage == clientMessage:
					accepted = 1
					gameState = 1
		
			except socket.timeout:
				clientSocket.sendto(clientMessage, (serverName, serverPort))
				attempts = attempts + 1

		if accepted == 0:
			print 'could not get server to respond'
			sys.exit()
		else:
			newGame(gameType)

		sent = 0

		# game is played here, this client is exceedingly simplistic and I have not
		# tested it along high traffic links that are likely to drop packets. It
		# worked on my home network, but that is to be expected. basically the client
		# alternates between receiving a move and acting upon it, dropping packets
		# may break the way I have made the client as I didnt test that very well.
		#
		# for an explanation of my protocol, see the README
		while(gameState > 0):
			try:
				if sent == 1:
					oldMessage = serverMessage
					serverMessage, twoTuple = clientSocket.recvfrom(1)

					# protocol is set up so that the server will resend the same
					# message until it gets what it expects, once it gets what
					# it expects it will send the correct message back. here I
					# ignore old messages, because they are forced timeouts.
	
					if oldMessage != serverMessage:
						sent = 0
					else:
						sent = 1

				if sent == 0:
					if serverMessage == 'E':
						gameBoard[int(serverDecision)] = serverSymbol
						printBoard()

						clientMessage = raw_input('Our turn, what is your move? (0-8): ')

						while(clientMessage not in accept):
							clientMessage = raw_input('Inappropriate move, what is your move? (0-8): ')
						clientSocket.sendto(clientMessage, (serverName, serverPort))
						sent = 1
					elif serverMessage == 'A':
						clientMessage = raw_input('Our turn, what is your move?: ')

						while(clientMessage not in accept):
							clientMessage = raw_input('Inappropriate move, what is your move? (0-8): ')
						clientSocket.sendto(clientMessage, (serverName, serverPort))
						sent = 1
					elif serverMessage == 'B':
						clientMessage = 'B'
						clientSocket.sendto('E', (serverName, serverPort))
						sent = 1
					elif serverMessage == 'I':
						print 'Server rejects the last move as invalid!'
						clientMessage = 'I'
						clientSocket.sendto(clientMessage, (serverName, serverPort))
						sent = 1
					elif serverMessage == 'W':
						clientMessage = raw_input('Still our turn, what is your move?: ')
						while(clientMessage not in accept):
							clientMessage = raw_input('Inappropriate move, what is your move? (0-8): ')
						clientSocket.sendto(clientMessage, (serverName, serverPort))
						sent = 1
					elif serverMessage == 'C':
						print 'Game Over: Server says that we win.'
						clientMessage = 'C'
						clientSocket.sendto(clientMessage, (serverName, serverPort))
						sent = 1
						gameState = 0
					elif serverMessage == 'S':
						print 'Game Over: Server says that it wins.'
						clientMessage = 'S'
						clientSocket.sendto(clientMessage, (serverName, serverPort))
						sent = 1
						gameState = 0
					elif serverMessage == 'D':
						print 'Game Over: Server says there was a draw.'
						clientSocket.sendto(clientMessage, (serverName, serverPort))
						sent = 1
						gameState = 0
					elif serverMessage.isdigit():
						# server sent their valid move, or confirmation of our move
						if clientMessage.isdigit():
							# server confirmed our move, send "E" to end our turn
							print clientMessage, ' confirmed, ending our turn.'
							gameBoard[int(clientMessage)] = clientSymbol
							printBoard()
							clientMessage = 'E'
							clientSocket.sendto(clientMessage, (serverName, serverPort))
							sent = 1
						else:
							# server sent their valid move, echo it back to confirm
							print 'Server wants to make valid move: ', serverMessage
							serverDecision = serverMessage
							clientMessage = serverMessage
							clientSocket.sendto(clientMessage, (serverName, serverPort))
							sent = 1
					else:
						clientSocket.sendto(clientMessage, (serverName, serverPort))
						sent = 1
			except socket.timeout:
				clientSocket.sendto(clientMessage, (serverName, serverPort))

		# END WHILE LOOP

		# game ended one way or another
		sys.exit()
	except socket.error:
		print 'error creating socket, exiting program.'
		sys.exit()

