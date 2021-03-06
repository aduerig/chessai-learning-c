#include "bitboard.hpp"
#include <stdlib.h>
#include <unordered_map>
#include <cmath>
#include <strings.h>
#include <fstream>

Engine::Engine() :
max_move_length(500), move_arr_size(500) {
    init_position();
    init_engine();
}

Engine::Engine(unsigned long long* board_data) :
max_move_length(500), move_arr_size(500) {
    init_position(board_data);
    init_engine();
}

void Engine::init_engine()
{
    compute_get_holders();
    move_list = (int*) malloc(move_arr_size * sizeof(int));

    move_stack = (int*) malloc((max_move_length + 1) * sizeof(int)); // +1 because pushing moves is nesseccary to chekc fo legality
    // lsb_lookup = (int*) malloc(64 * sizeof(int));
    stack_index = -1;

    in_check = false;
    init_masks();
    init_lsb_lookup();

    // square precomputed mask things
    fill_square_masks();
}

void Engine::reset_engine()
{
    stack_index = -1;
    in_check = false;
    init_position();
    compute_get_holders();
}

void Engine::init_position()
{
    pos.white_pawns = 0b0000000000000000000000000000000000000000000000001111111100000000; // 65280
    pos.white_rooks = 0b0000000000000000000000000000000000000000000000000000000010000001; // 129
    pos.white_nights = 0b0000000000000000000000000000000000000000000000000000000001000010; // 66
    pos.white_bishops = 0b0000000000000000000000000000000000000000000000000000000000100100; // 36
    pos.white_queens = 0b0000000000000000000000000000000000000000000000000000000000001000; // 8
    pos.white_kings = 0b0000000000000000000000000000000000000000000000000000000000010000; // 16

    pos.black_pawns = 0b0000000011111111000000000000000000000000000000000000000000000000; // 71776119061217280
    pos.black_rooks = 0b1000000100000000000000000000000000000000000000000000000000000000; // 9295429630892703744
    pos.black_nights = 0b0100001000000000000000000000000000000000000000000000000000000000; // 4755801206503243776
    pos.black_bishops = 0b0010010000000000000000000000000000000000000000000000000000000000; // 2594073385365405696
    pos.black_queens = 0b0000100000000000000000000000000000000000000000000000000000000000; // 576460752303423488
    pos.black_kings = 0b0001000000000000000000000000000000000000000000000000000000000000; // 1152921504606846976
}

void Engine::init_position(unsigned long long *board_data)
{
    pos.white_pawns = board_data[0];
    pos.white_rooks = board_data[1];
    pos.white_nights = board_data[2];
    pos.white_bishops = board_data[3]; 
    pos.white_queens = board_data[4]; 
    pos.white_kings = board_data[5]; 

    pos.black_pawns = board_data[6];
    pos.black_rooks = board_data[7];  
    pos.black_nights = board_data[8];  
    pos.black_bishops = board_data[9]; 
    pos.black_queens = board_data[10]; 
    pos.black_kings = board_data[11];
}

void Engine::init_lsb_lookup()
{
    for(int i=0;i<64;i++)
    {
        lsb_lookup.insert({(unsigned long long) std::pow(2,i),i});
    }
}

void Engine::clean_up()
{
    free(move_list);
    free(move_stack);
    free(row_mask);
    free(col_mask);
    free(diag_left_mask);
    free(diag_right_mask);
}


void Engine::init_masks()
{

    row_mask = (unsigned long long*) malloc(8 * sizeof(unsigned long long));    
    fill_row_mask_arr();

    col_mask = (unsigned long long*) malloc(8 * sizeof(unsigned long long));
    fill_col_mask_arr();

    // Diag left masks start on left side and moves from left to right, top to bottom
    // [0] corresponds to bottom left corner
    // [0]-[7] moves up y axis along x=0
    // [7] is top left corner
    // [7]-[14] moves across x-axis along y=7
    // [14] is top right corner
    diag_left_mask = (unsigned long long*) malloc(15 * sizeof(unsigned long long));
    fill_diag_left_mask_arr();

    // Diag right masks start on bottom side and moves from left to right, bottom to top
    // [0] corresponds to bottom right corner
    // [0]-[7] moves down the x axis along y=0
    // [7] is bottom left corner
    // [7]-[14] moves up the y-axis along x=0
    // [14] is top left corner
    diag_right_mask = (unsigned long long*) malloc(15 * sizeof(unsigned long long));
    fill_diag_right_mask_arr();
}

unsigned long long Engine::make_col_mask(unsigned long long mask)
{
    for(int i = 0; i < 7; i++)
    {
        mask = mask | mask << 8;
    }
    return(mask);
}

void Engine::fill_col_mask_arr()
{
    for(int i = 0; i < 8; i++)
    {
        col_mask[i] = make_col_mask(1ULL << i);
    }
}

unsigned long long Engine::make_row_mask(unsigned long long mask)
{
    for(int i = 0; i < 7; i++)
    {
        mask = mask | mask << 1;
    }
    return(mask);
}

void Engine::fill_row_mask_arr()
{
    for(int i = 0; i < 8; i++)
    {
        row_mask[i] = make_row_mask(1ULL << 8*i);
    }
}

unsigned long long Engine::make_diag_left_mask(unsigned long long mask)
{
    unsigned long long BR_mask = ~(row_mask[0] | col_mask[7]); //BR standss for bottom right maybe? Look into
    for(int i = 0; i < 8; i++)
    {
        mask = mask | ((mask & BR_mask) >> 7);
    }
    return(mask);
}


void Engine::fill_diag_left_mask_arr()
{
    unsigned long long mask_base = 1;

    // for(int i = 0; i < 8; i++)
    // {
    //     diag_left_mask[i] = make_diag_left_mask(mask_base);
    //     if(i != 7)
    //     {
    //         mask_base = mask_base << 8;
    //     }
    // }
    // mask_base = mask_base << 1;

    for(int i = 0; i < 7; i++)
    {
        diag_left_mask[i] = make_diag_left_mask(mask_base);
        mask_base = mask_base << 8;
    }

    diag_left_mask[7] = make_diag_left_mask(mask_base);
    mask_base = mask_base << 1;

    for(int j = 8; j < 15; j++)
    {
        diag_left_mask[j] = make_diag_left_mask(mask_base);
        mask_base = mask_base << 1;
    }
}


unsigned long long Engine::make_diag_right_mask(unsigned long long mask)
{
    unsigned long long TR_mask = ~((row_mask[7]) | (col_mask[7])); //TR standss for top right maybe? Look into
    for(int i = 0; i < 8; i++)
    {
        mask = mask | ((mask & TR_mask) << 9);
    }
    return(mask);
}


void Engine::fill_diag_right_mask_arr()
{
    unsigned long long mask_base = 1ULL << 7;
    
    // for(int i = 0; i < 8; i++)
    // {
    //     diag_right_mask[i] = make_diag_right_mask(mask_base);
    //     if(i != 7)
    //     {
    //         mask_base = mask_base >> 1;
    //     } 
    // }

    for(int i = 0; i < 7; i++)
    {
        diag_right_mask[i] = make_diag_right_mask(mask_base);
        mask_base = mask_base >> 1;
    }

    diag_right_mask[7] = make_diag_right_mask(mask_base);
    mask_base = mask_base << 8;

    for(int j = 8; j < 15; j++)
    {
        diag_right_mask[j] = make_diag_right_mask(mask_base);
        mask_base = mask_base << 8;
    }          
}

void Engine::fill_square_masks()
{
    U64 temp;

    for(int i = 0; i < 64; i++)
    {
        temp = 1ULL << i;

        int diag = get_diag(get_rank(temp), get_file(temp));
        int left_diag = diag >> 5;
        int right_diag = diag & 0x000000000000000F;

        square_masks[i].left_diag_mask_excluded = ~temp & diag_left_mask[left_diag];
        square_masks[i].right_diag_mask_excluded = ~temp & diag_right_mask[right_diag];
        square_masks[i].file_mask_excluded = ~temp & col_mask[get_file(temp)];

        square_masks[i].file_mask = col_mask[get_file(temp)];
        square_masks[i].rank_mask = row_mask[get_rank(temp)];
        // printf("\n");
        // print_bit_rep(square_masks[i].left_diag_mask_excluded);
    }
}

int Engine::get_max_move_length()
{
    return(max_move_length);
}

// old
// unsigned long long Engine::get_all()
// {
//     return(get_all_white() | get_all_black());
// }

// unsigned long long Engine::get_all_white()
// {
//     return(pos.white_pawns | pos.white_rooks | pos.white_nights | pos.white_bishops | pos.white_kings | pos.white_queens);
// }


// unsigned long long Engine::get_all_black()
// {
//     return(pos.black_pawns | pos.black_rooks | pos.black_nights | pos.black_bishops | pos.black_kings | pos.black_queens);
// }

unsigned long long Engine::get_all_white()
{
    return(get_white_holder);
}


unsigned long long Engine::get_all_black()
{
    return(get_black_holder);
}


// Andrew's Linux Labtop: with flags: ~11.7ns without: ~32ns (probably due to inlining)
unsigned long long Engine::get_all()
{
    return(get_all_holder);
}


// Takes in a 64 bit number with single bit
// Returns the rank piece is on 0-7, bottom to top
// Andrew's Linux Labtop: with flags: ~6ns without: ~16.5ns (idk why)
int Engine::get_rank(unsigned long long const num)
{
    unsigned long long max_rank_1_value = 128ULL; // 2^7
    if(num <= max_rank_1_value)
    {
        return(0);
    }

    unsigned long long max_rank_2_value = 32768ULL; // 2^15
    if(num <= max_rank_2_value)
    {
        return(1);
    }

    unsigned long long max_rank_3_value = 8388608ULL; // 2^23
    if(num <= max_rank_3_value)
    {
        return(2);
    }

    unsigned long long max_rank_4_value = 2147483648ULL; // 2^31
    if(num <= max_rank_4_value)
    {
        return(3);
    }

    unsigned long long max_rank_5_value = 549755813888ULL; // 2^39
    if(num <= max_rank_5_value)
    {
        return(4);
    }

    unsigned long long max_rank_6_value = 140737488355328ULL; // 2^47
    if(num <= max_rank_6_value)
    {
        return(5);
    }

    unsigned long long max_rank_7_value = 36028797018963968ULL; // 2^55
    if(num <= max_rank_7_value)
    {
        return(6);
    }

    unsigned long long max_rank_8_value = 9223372036854775808ULL; // 2^63
    if(num <= max_rank_8_value)
    {
        return(7);
    }
    return -1;
}


// Takes in a 64 bit number with single bit
// Returns the file piece is on 0-7, left to right
// ~9ns
int Engine::get_file(unsigned long long const num)
{
    switch(num)
    {
        //2^[0, 8, 16, 24, 32, 40, 48, 56]
        case 1ULL:
        case 256ULL:
        case 65536ULL:
        case 16777216ULL:
        case 4294967296ULL:
        case 1099511627776ULL:
        case 281474976710656ULL:
        case 72057594037927936ULL:
            return(0);

        //2^[1,9,17,25,33,41,49,57]
        case 2ULL:
        case 512ULL:
        case 131072ULL:
        case 33554432ULL:
        case 8589934592ULL:
        case 2199023255552ULL:
        case 562949953421312ULL:
        case 144115188075855872ULL:
            return(1);

        // 2^[2,10,18,26,34,42,50,58]
        case 4ULL:
        case 1024ULL:
        case 262144ULL:
        case 67108864ULL:
        case 17179869184ULL:
        case 4398046511104ULL:
        case 1125899906842624ULL:
        case 288230376151711744ULL:
            return(2);

        // 2^[3,11,19,27,35,43,51,59]    
        case 8ULL:
        case 2048ULL:
        case 524288ULL:
        case 134217728ULL:
        case 34359738368ULL:
        case 8796093022208ULL:
        case 2251799813685248ULL:
        case 576460752303423488ULL:
            return(3);

        // 2^[4,12,20,28,36,44,52,60]
        case 16ULL:
        case 4096ULL:
        case 1048576ULL:
        case 268435456ULL:
        case 68719476736ULL:
        case 17592186044416ULL:
        case 4503599627370496ULL:
        case 1152921504606846976ULL:
            return(4);

        // 2^[5,13,21,29,37,45,53,61]
        case 32ULL:
        case 8192ULL:
        case 2097152ULL:
        case 536870912ULL:
        case 137438953472ULL:
        case 35184372088832ULL:
        case 9007199254740992ULL:
        case 2305843009213693952ULL:
            return(5);

        // 2^[6, 14, 22, 30, 38, 46, 54, 62]
        case 64ULL:
        case 16384ULL:
        case 4194304ULL:
        case 1073741824ULL:
        case 274877906944ULL:
        case 70368744177664ULL:
        case 18014398509481984ULL:
        case 4611686018427387904ULL:
            return(6);

        // 2^[7,15,23,31,39,47,55,63]
        case 128ULL:
        case 32768ULL:
        case 8388608ULL:
        case 2147483648ULL:
        case 549755813888ULL:
        case 140737488355328ULL:
        case 36028797018963968ULL:
        case 9223372036854775808ULL:
            return(7);

        default:
            return(-1);
    }
}

// Andrew's Linux Labtop: with flags: ~6ns without: ~16ns (probs inline)
int Engine::get_diag(int const rank, int const file)
{
    int rank_file_sum = rank+file;
    int right;

    //Total val also equals left diag index

    if(rank > file) //Above the middle diagonal line r = 7
    {
        right = 7+(rank_file_sum-2*file);
    }
    else //Below middle line
    {
        right = 7-(rank_file_sum-2*rank);
    }

    // int* diag = (int*) malloc(2 * sizeof(int));
    // diag[0] = rank_file_sum;
    // diag[1] = right;
    // int ret_val = (rank_file_sum << 5) | (rank_file_sum << 5);


    return(rank_file_sum << 5) | (right);
}

// Takes in move information
//     start int 0-63 
// Square moved piece started on
// //     end int 0-63 
// Square moved piece ended on
// //     m_type int 0-63 
// Type of move made
// //     piece int 0-4 
// Piece taken in move
// //     promotion int 2-5 
// Piece to promote pawn to
// Return an int representing all above info
int Engine::encode_move(int const start, int const end, int const m_type, int const piece, int const promotion, int const color)
{
    int encode_start = start;
    int encode_end = end << 6;
    int encode_type = m_type << 12;
    int encode_piece = piece << 14;
    int encode_promotion = promotion << 17;
    int encode_color = color << 20;
    return(encode_start | encode_end | encode_type | encode_piece | encode_promotion | encode_color);
}

// Takes in an int move
// Returns square number the moved piece originated from
int Engine::decode_from(int const move)
{
    return(move & 63);
}

// Takes in an int move
// Returns square number moved piece travels to
int Engine::decode_to(int const move)
{
    return((move >> 6) & 63);
}

// Takes in an int move
// Returns type of move made
int Engine::decode_type(int const move)
{
    return((move >> 12) & 3);
}   

// Takes in an int move
// Returns any piece taken by move
int Engine::decode_piece(int const move)
{
    return((move >> 14) & 7);
}

// Takes in an int move
// Returns new piece pawn promoted to
int Engine::decode_promo(int const move)
{
    return((move >> 17) & 3);
}

// Takes in an int move
// Returns color
int Engine::decode_color(int const move)
{
    return(move >> 20);
}

std::string Engine::piece_type_to_string(int const piece)
{
    switch(piece)
    {
        case PAWN:
            return("pawn");

        case ROOK:
            return("rook");

        case NIGHT:
            return("night");

        case BISHOP:
            return("bishop");

        case QUEEN:
            return("queen");

        case KING:
            return("king");

        case NONE:
            return("nothing");

        default:
            return("WARNING, PIECE INFO IS GARBAGE");
    }
}

// Takes in a move to be added to the move stack
// Returns nothing
// Alters the move stack and stack_index value
void Engine::stack_push(int const move)
{
    // get pointer to stack index
    // get pointer to move_stack
    move_stack[++stack_index] = move;
}

// Takes in nothing
// Returns the last move in the move stack
// Alters the stack_index value
int Engine::stack_pop()
{
    // get pointer to stack index
    // get pointer to move_stack
    return(move_stack[stack_index--]);
}

// Takes in a move, alters the BitboardEngine's representation to the NEXT state based on the CURRENT move action
// Currently 
void Engine::push_psuedo_move(int move)
{
    stack_push(move);
    int move_type = decode_type(move);
    int promo_type = decode_promo(move);
    int original_square = decode_from(move);
    int end_square = decode_to(move);
    int taken_piece_type = decode_piece(move);
    int color;

    unsigned long long curr_piece_loc = square_to_bitboard(original_square);
    unsigned long long taken_piece_loc = square_to_bitboard(end_square);

    color = decode_color(move);

    int curr_piece_type = get_piece_by_bitboard(color, curr_piece_loc);
    remove_piece(color, curr_piece_type, curr_piece_loc);

    if(taken_piece_type)
    {
        remove_piece(1-color, taken_piece_type, taken_piece_loc);
    }

    if(move_type == PROMOTION)
    {
        place_piece(color, promo_type, taken_piece_loc);
    }
    else
    {
        place_piece(color, curr_piece_type, taken_piece_loc);
    }
    compute_get_holders();
}

void Engine::push_move(int move)
{
    push_psuedo_move(move);
}

// Takes in a move, alters the BitboardEngine's representation to the PREVIOUS state based on the LAST move action
void Engine::pop_psuedo_move()
{
    int move = stack_pop();

    int original_square = decode_from(move);
    int end_square = decode_to(move);
    int taken_piece_type = decode_piece(move);
    int color;

    unsigned long long curr_piece_loc = square_to_bitboard(end_square);
    unsigned long long orig_piece_loc = square_to_bitboard(original_square);

    color = decode_color(move);

    int curr_piece_type = get_piece_by_bitboard(color, curr_piece_loc);
    remove_piece(color, curr_piece_type, curr_piece_loc);

    if(taken_piece_type)
    {
        place_piece(1-color, taken_piece_type, curr_piece_loc);
    }

    place_piece(color, curr_piece_type, orig_piece_loc);
    compute_get_holders();
}

void Engine::pop_move()
{
    pop_psuedo_move();
}

void Engine::compute_get_holders()
{
    get_white_holder = pos.white_pawns | pos.white_rooks | pos.white_nights | pos.white_bishops | pos.white_kings | pos.white_queens;
    get_black_holder = pos.black_pawns | pos.black_rooks | pos.black_nights | pos.black_bishops | pos.black_kings | pos.black_queens;
    get_all_holder = get_white_holder | get_black_holder;
}

// East << 1
// Southeast >> 7
// South >> 8
// Southwest >> 9
// West >> 1
// Northwest << 7
// North << 8
// Northeast << 9

// ~3ns
unsigned long long Engine::square_to_bitboard(int const square)
{
    return(1ULL << square);
}

// ~3ns
unsigned long long Engine::get_bitboard_of_piece(Piece piece, int const color)
{
    if(color == WHITE)
    {
        if(piece == KING)
        {
            return pos.white_kings;
        }
        else
        {
            std::cout << "WARNING RETURNING GARBAGE IN GET_SQUARE";
            return(-1);
        }
    }
    // black
    else
    {
        if(piece == KING)
        {
            return pos.black_kings;
        }
        else
        {
            std::cout << "WARNING RETURNING GARBAGE IN GET_SQUARE";
            return(-1);
        }
    }
}

// 15ns
Piece Engine::get_piece_by_bitboard(int color, unsigned long long board)
{
    unsigned long long pawns, rooks, nights, bishops, queens, kings;
    if(color == WHITE)
    {
        pawns = pos.white_pawns;
        rooks = pos.white_rooks;
        nights = pos.white_nights;
        bishops = pos.white_bishops;
        queens = pos.white_queens;
        kings = pos.white_kings;
    }
    else
    {
        pawns = pos.black_pawns;
        rooks = pos.black_rooks;
        nights = pos.black_nights;
        bishops = pos.black_bishops;
        queens = pos.black_queens;
        kings = pos.black_kings;
    }

    if(board & pawns)
    {
        return(PAWN);
    }
    else if(board & rooks)
    {
        return(ROOK);
    }
    else if(board & nights)
    {
        return(NIGHT);
    }
    else if(board & bishops)
    {
        return(BISHOP);
    }
    else if(board & queens)
    {
        return(QUEEN);
    }
    else if(board & kings)
    {
        return(KING);
    }
    else
    {
        return(NONE);
    }
}

// 30ns
Piece Engine::get_piece_by_bitboard(unsigned long long board)
{
    return(get_piece_by_bitboard(get_color_by_bitboard(board), board));
}

void Engine::remove_piece(int color, int type, unsigned long long board)
{
    if(color == WHITE)
    {
        switch(type)
        {
            case PAWN:
                pos.white_pawns = pos.white_pawns - board;
                return;
            case ROOK:
                pos.white_rooks = pos.white_rooks - board;
                return;
            case NIGHT:
                pos.white_nights = pos.white_nights - board;
                return;
            case BISHOP:
                pos.white_bishops = pos.white_bishops - board;
                return;
            case QUEEN:
                pos.white_queens = pos.white_queens - board;
                return;
            case KING:
                pos.white_kings = pos.white_kings - board;
                return;
        }
    }
    else
    {
        switch(type)
        {
            case PAWN:
                pos.black_pawns = pos.black_pawns - board;
                return;
            case ROOK:
                pos.black_rooks = pos.black_rooks - board;
                return;
            case NIGHT:
                pos.black_nights = pos.black_nights - board;
                return;
            case BISHOP:
                pos.black_bishops = pos.black_bishops - board;
                return;
            case QUEEN:
                pos.black_queens = pos.black_queens - board;
                return;
            case KING:
                pos.black_kings = pos.black_kings - board;
                return;
        }
    }
}

void Engine::place_piece(int color, int type, unsigned long long board)
{
    if(color == WHITE)
    {
        switch(type)
        {
            case PAWN:
                pos.white_pawns = pos.white_pawns | board;
                return;
            case ROOK:
                pos.white_rooks = pos.white_rooks | board;
                return;
            case NIGHT:
                pos.white_nights = pos.white_nights | board;
                return;
            case BISHOP:
                pos.white_bishops = pos.white_bishops | board;
                return;
            case QUEEN:
                pos.white_queens = pos.white_queens | board;
                return;
            case KING:
                pos.white_kings = pos.white_kings | board;
                return;
        }
    }
    else
    {
        switch(type)
        {
            case PAWN:
                pos.black_pawns = pos.black_pawns | board;
                return;
            case ROOK:
                pos.black_rooks = pos.black_rooks | board;
                return;
            case NIGHT:
                pos.black_nights = pos.black_nights | board;
                return;
            case BISHOP:
                pos.black_bishops = pos.black_bishops | board;
                return;
            case QUEEN:
                pos.black_queens = pos.black_queens | board;
                return;
            case KING:
                pos.black_kings = pos.black_kings | board;
                return;
        }
    }
}

int Engine::get_color_by_bitboard(unsigned long long const board)
{
    if(board & get_all_white())
    {
        return(WHITE);
    }
    else
    {
        return(BLACK);
    }
}

unsigned long long* Engine::get_board_rep()
{
    unsigned long long* rep = (unsigned long long*) malloc(12 * sizeof(unsigned long long));
    rep[0] = pos.white_pawns;
    rep[1] = pos.white_rooks;
    rep[2] = pos.white_nights;
    rep[3] = pos.white_bishops;
    rep[4] = pos.white_queens;
    rep[5] = pos.white_kings;

    rep[6] = pos.black_pawns;
    rep[7] = pos.black_rooks;
    rep[8] = pos.black_nights;
    rep[9] = pos.black_bishops;
    rep[10] = pos.black_queens;
    rep[11] = pos.black_kings;
    return rep;
}

// Some hueristics have been met, the only way to check if a move is legal or not, we must make it.
bool Engine::check_legal(int move, int color)
{
    // return false;
    // std::cout << "checking legality of: " << move << std::endl;
    // print_move_info(move);
    push_move(move);
    // print_chess_char();
    if(get_in_check(color))
    {
        // std::cout << "popping, NOT LEGAL: " << move << std::endl;
        // print_chess_char();
        pop_move();
        // print_chess_char();
        return(false);
    }
    // std::cout << "popping, LEGAL: " << move << std::endl;
    // print_chess_char();
    pop_move();
    // print_chess_char();
    return(true);
}


// -1 for not over
// 0 for black winning
// 1 for white winning
// 2 for draw
int Engine::is_terminal(int color, int* move_list)
{
    int number_of_possible_moves = move_list[0];
    if(number_of_possible_moves == 0)
    {
        // std::cout << "TERM > " << color << " is out of moves.\n";
        if(get_in_check(color))
        {
            // std::cout << "TERM > " << color << " is in check.\n";
            return(1-color);
        }
        return(2);
    }
    return(-1);
}

bool Engine::get_in_check(int color)
{
    unsigned long long my_king;
    unsigned long long enemy_pawns, enemy_rooks, enemy_nights, enemy_bishops, enemy_queens, enemy_kings;
    unsigned long long pawn_attackers, rook_attackers, night_attackers, bishop_attackers, king_attackers;

    if(color == WHITE)
    {
        my_king = pos.white_kings;

        enemy_pawns = pos.black_pawns;
        enemy_rooks = pos.black_rooks;
        enemy_nights = pos.black_nights;
        enemy_bishops = pos.black_bishops;
        enemy_queens = pos.black_queens;
        enemy_kings = pos.black_kings;

        pawn_attackers = white_pawn_attacks(my_king) & enemy_pawns;
    }
    else
    {
        my_king = pos.black_kings;

        enemy_pawns = pos.white_pawns;
        enemy_rooks = pos.white_rooks;
        enemy_nights = pos.white_nights;
        enemy_bishops = pos.white_bishops;
        enemy_queens = pos.white_queens;
        enemy_kings = pos.white_kings;

        pawn_attackers = black_pawn_attacks(my_king) & enemy_pawns;
    }

    if(pawn_attackers)
    {
        return true;
    }

    rook_attackers = one_rook_attacks(my_king) & (enemy_rooks | enemy_queens);
    if(rook_attackers)
    {
        return true;
    }

    night_attackers = night_attacks(my_king) & enemy_nights;
    if(night_attackers)
    {
        return true;
    }

    bishop_attackers = one_bishop_attacks(my_king) & (enemy_bishops | enemy_queens);
    if(bishop_attackers)
    {
        return true;
    }

    king_attackers = king_attacks(my_king) & enemy_kings;
    if(king_attackers)
    {
        return true;
    }
    return false;
}


// will store how many attackers in info[0], attacker squares in info[1], and blocking squares in info[2]
unsigned long long* Engine::get_attackers_blocks(int const color)
{
    unsigned long long my_king;
    unsigned long long enemy_pawns, enemy_rooks, enemy_nights, enemy_bishops, enemy_queens;
    unsigned long long pawn_attackers, rook_attackers, night_attackers, bishop_attackers;
    unsigned long long card_attacks, diag_attacks;

    unsigned long long* info = (unsigned long long*) calloc(3, sizeof(unsigned long long));

    if(color == WHITE)
    {
        my_king = pos.white_kings;

        enemy_pawns = pos.black_pawns;
        enemy_rooks = pos.black_rooks;
        enemy_nights = pos.black_nights;
        enemy_bishops = pos.black_bishops;
        enemy_queens = pos.black_queens;

        pawn_attackers = white_pawn_attacks(my_king) & enemy_pawns;
    }
    else
    {
        my_king = pos.black_kings;

        enemy_pawns = pos.white_pawns;
        enemy_rooks = pos.white_rooks;
        enemy_nights = pos.white_nights;
        enemy_bishops = pos.white_bishops;
        enemy_queens = pos.white_queens;

        pawn_attackers = black_pawn_attacks(my_king) & enemy_pawns;
    }

    // i believe it is impossible to hit more than one check with any of these 4 ifs

    if(pawn_attackers)
    {
        info[0]++;
        info[1] = pawn_attackers;
    }

    card_attacks = one_rook_attacks(my_king);
    rook_attackers = card_attacks & (enemy_rooks | enemy_queens);
    if(rook_attackers)
    {
        info[0]++;
        info[1] = rook_attackers;
        info[2] = card_attacks & one_rook_attacks(rook_attackers);
    }

    diag_attacks = one_bishop_attacks(my_king);
    bishop_attackers = diag_attacks & (enemy_bishops | enemy_queens);
    if(bishop_attackers)
    {
        info[0]++;
        info[1] = bishop_attackers;
        info[2] = diag_attacks & one_bishop_attacks(bishop_attackers);
    }

    night_attackers = night_attacks(my_king) & enemy_nights;
    if(night_attackers)
    {
        info[0]++;
        info[1] = night_attackers;
    }
    
    // king_attackers = king_attacks(my_king) & enemy_kings;
    
    return(info);
}