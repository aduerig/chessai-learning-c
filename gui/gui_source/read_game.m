function [game_data, tot_moves] = read_game(gNum)
%read_game Takes in a game number and returns a matrix of all the moves and
%the total number of moves made
%
%   Takes in gNum, game number corresponding to read file
%
%   Returns game_data, a 12xmoves matrix. Each column represents a
%   boardstate at a given move.
%   
%   Returns tot_moves, the total number of boardstates/moves throughout the game. Move
%   0 corresponds to a newly setup board.

fileID = fopen('examplegame.txt');
pieces = 12;
base = ['%d, '];
fmt = [repmat(base, 1, pieces-1) '%d\n'];
[raw_data, count] = fscanf(fileID, fmt);
tot_moves = count/pieces;
game_data = reshape(raw_data,pieces,tot_moves);
fclose(fileID);
end