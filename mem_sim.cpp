/*
  	Author: Malhar Jajoo
 *	Year: EIE 2nd Year

 */


#include <iostream>
#include <vector>
#include <stdint.h> // for uint32_t
#include <stdlib.h> // for atoi
#include <stdio.h> // for fprintf and stderr.
#include <iomanip> // for manipulating I/O streams.
#include <string>
#include <cmath>
#include <sstream>
#include <fstream>
#include <algorithm>

using namespace std;

/* Following is the code for a cache simulator -
1. The configuration of the cache is given as command line arguments.
2. The input are the memory transactions through stdin.
3. Output is the response on stdout.

Input parameters (in order of appearance on the command line) are:

Address bits

Bytes/Word

Words/Block

Blocks/Set

Sets/Cache

Hit time (cycles/Access) - Use this value in calculations.
Memory read time in cycles/Block - Use this value in calculations.
Memory write time in cycles/Block - Use this value in calculations.

Write Policy - Write-back
Replacement Policy - Least Recently Used ( LRU ) .

 */
 //============================================
    // GLOBAL CONFIG BITS

uint32_t address_bits ;
uint32_t bytes_per_word  ;  // power of 2
uint32_t words_per_block  ; // power of 2
uint32_t blocks_per_set ; // power of 2
uint32_t sets_per_cache ; // power of 2
int hit_time_in_cycles_per_hit ;
int memory_read_time_in_cycles_per__block ;
int memory_write_time_in_cyces_per_block ;

// extra global variable - because memory is organized into blocks  ( Since we never need to go below the granularity of a block....)
// If we do need to , we first fetch in cache and then use words.
uint32_t blocks_per_main_memory;



//============== User defined data structures =====================
 
// Use a struct since a cache needs to store information related to 
// tag bits 
// valid bit , dirty flag
// LRU counter 
struct block_cache
{
	// chose to use std::vector<int> instead of something like
	// std::vector<uint8_t> 
	std::vector<int> word ; 
	std::vector < std::vector<int> > block_vector ; // a block is a vector of words
	uint32_t block_tag ;
	bool valid_bit ;
	bool dirty_flag ;

	uint32_t dirty_byte_address ; // this is required. see Flush_Response for usage.
	uint32_t lru_counter ;

	// NOTE - The tag has been set to a zero - non-coherent useless value.
	// The cache contents are initialized to all zeros
	block_cache():word(::bytes_per_word,0),block_vector(::words_per_block,word) ,block_tag(0),valid_bit(false),dirty_flag(false),dirty_byte_address(0),lru_counter(0)
	{
		std::cerr << " Cache block has now been initialized." << std::endl ;
	}

};

struct block_main_mem
{
	
	std::vector < std::vector<int> > block_vector ;


	block_main_mem():block_vector(::words_per_block,vector<int>(::bytes_per_word,0))
	{
		fprintf (stderr, "Main memory block has now been initialized \n\n" );
	}
};


// ============ function prototypes ===========================================================s

void Read_response(int word_index,int set_index,uint32_t tag_bits,vector< vector<block_cache> >& cache,vector<block_main_mem> &main_mem ,uint32_t block_address);
void Write_response(int word_index,int set_index,uint32_t tag_bits,vector< vector<block_cache> >& cache,vector<block_main_mem> &main_mem ,uint32_t block_address,uint32_t address,string  data);

void Flush_response(vector< vector<block_cache> >& cache,vector<block_main_mem> &main_mem) ;
void Debug_response( const vector< vector<block_cache> > &cache);

void cache_initialization( vector< vector<block_cache> > &cache ) ;
void memory_initialization( vector<block_main_mem> &main_mem );

void calculate_location_using_address(uint32_t address,int &word_index,int& set_index,uint32_t &tag_bits,uint32_t& block_address);

string get_data( const vector< vector<block_cache> > &cache, int word_index,int set_index,int extra_index ) ;

void put_data(  vector< vector<block_cache> > &cache, int word_index,int set_index,int extra_index ,string data);

void check_hit(const vector< vector<block_cache> > &cache,int set_index,uint32_t tag_bits,int &hit_block_index,bool &hit ) ;

void create_dashes(int set_index );

void printReadAck(bool hit, int set_index, int time,std::string data);

void printWriteAck(bool hit, int set_index, int time);

int calculateTime(int blocks_written_to_memory_counter,int blocks_read_from_memory_counter);

//======================================================================================================

/*
 1) Takes as input the configuration bits from stdin and creates cache and memory 
 2) Handles different types of requests ( see specification for all the types of the request )
 from the stdin in a simple while loop.

See the appropriate functions for each request for their working 
*/

int main(int argc, char* argv[])
{
	// stores configuration bits in the globals
	fprintf(stderr,"Storing config bits......\n") ;

	(::address_bits) = atoi(argv[1]) ;

	(::bytes_per_word) = atoi(argv[2]) ;  // power of 2

	(::words_per_block) =  atoi(argv[3]) ; // power of 2

	(::blocks_per_set) =  atoi(argv[4]) ; // power of 2
	(::sets_per_cache) =  atoi(argv[5]) ; // power of 2

	(::hit_time_in_cycles_per_hit)  =  atoi(argv[6]) ;
	(::memory_read_time_in_cycles_per__block) =  atoi(argv[7]) ;
	(::memory_write_time_in_cyces_per_block)  = atoi(argv[8]) ;

	std::cerr << "Finished storing config bits" << std::endl << std::endl ;


	//initialization
	std::vector< std::vector<block_cache> > cache ;
	std::vector<block_main_mem> main_mem ;

	cache_initialization(cache) ;
	memory_initialization(main_mem) ;



    while(true)
    {
             string cmd ;
             std::cin >> cmd ;  //stdin
            cerr << "COMMAND=" << cmd << endl ;

             if(std::cin.eof())
            {
                std::cerr << " Reached EOF.Terminating Loop" << std::endl ;
                break ;
            }

             if(std::cin.fail())
             {
                 std::cout << " You have entered an incorrect(non string) input." << std::endl ;
                 std::cout << " Aborting !" << std::endl ;
                 break ;
             }


              if(cmd=="flush-req")
             {
                    std::cerr << "Initiating flush-req ......" << std::endl ;

                    Flush_response(cache,main_mem) ;

                    std::cerr << " End of flush-req" << std::endl << std::endl ;
                    std::cerr << "=========================" << std::endl ;
             }

            else if(cmd=="read-req")
             {

                     uint32_t address ;
                     uint32_t block_address ; // NOTE - We need this to pass it to Read Response so that it can avoid extra calculation.
                     int word_index,set_index;
                     uint32_t tag_bits;

                    std::cin >> std::dec >>  address ;
                    std::cerr << std::dec <<  "Main address=" << address << endl ;
                    calculate_location_using_address(address,word_index,set_index,tag_bits,block_address);
                    std::cerr << " Initiating read-req ...." << std::endl ;

                    Read_response(word_index,set_index,tag_bits,cache,main_mem,block_address) ; // set index for cache and main memory are calculated inside read_response.

                    std::cerr << " End of read-req" << std::endl << std::endl;
                    std::cerr << "===================================" << std::endl ;
             }

              else if(cmd=="write-req")
              {
                    uint32_t address ;
                    uint32_t block_address ; // NOTE - We need this to pass it to Read Response so that it can avoid extra calculation.
                    int word_index,set_index;
                    uint32_t tag_bits;

                    std::cin >> std::dec >> address ;
                    std::cerr << std::dec << "Main address="  <<address << std::endl  ;
                    calculate_location_using_address(address,word_index,set_index,tag_bits,block_address);

                    std::cerr << "Initiating write-req......." << std::endl ;
                    string data ;
                    std::cin >> data ;
                    std::cerr << "Data I receive " << data << endl ;

                    /*string data ;
                    cin >> data ;

                    if(data.size() != 2*bytes_per_word)
                    {
                    cout << " Incorrect data. Cannot write" << endl ;
                    cout << "Exiting " << endl ;
                    return 0;
                    }*/

                    Write_response(word_index,set_index, tag_bits, cache,main_mem , block_address, address, data);

                    std::cerr << " End of write-req" << std::endl << std::endl ;
                    std::cerr << "=========================" << std::endl ;
              }




            else if(cmd == "debug-req")
            {
                std::cerr << "Initiating debug-req......." << std::endl ;

                 Debug_response(cache) ;

                 std::cerr << " End of debug-req" << std::endl << std::endl;
                 std::cerr << "=========================" << std::endl ;
            }

            else if(cmd=="#")
            {
                cerr << "removing comment..... " << endl ;
                string skip_this_line ;
                getline(cin,skip_this_line) ;
                cerr << "Removed comment" << endl ;
            }

             else
             {
                cout << " Invalid input. " << endl ;
             }



    }

    std::cerr << " Program finished" << std::endl ;
    return 0;
}






// ========================== MEMORY RELATED FUNCTUONS ============================

void memory_initialization( vector<block_main_mem> &main_mem )
{

    int total_bytes = int(pow(2,address_bits)) ;

    // NOTE - this calculation ensures that the write requests will be redirected to a unique address in the memory.
    //         We are using this EXHAUSTIVE approach because we have organized memory this way.
    (::blocks_per_main_memory) = (total_bytes)/( (::bytes_per_word) * (::words_per_block) ) ;
   // cout << "Total blocks =" << ::blocks_per_main_memory << endl ;

    block_main_mem b ;
    main_mem.resize(blocks_per_main_memory,b)   ;

}

// ===================== Cache Related Functions ===================================


void cache_initialization( vector< vector<block_cache> > &cache )
{

    block_cache b ;
    std::vector<block_cache> sets(blocks_per_set,b) ;

  //  std::vector< vector<block> > cache(sets_per_cache,sets)   ;
    cache.resize(sets_per_cache,sets)   ;


}
 
// ====================  calculation of location within cache  =====================

// Input - Byte Address 
// Ouptut - As refernce parameters - the set index , word index , and the tag bits

// Procedure - The input address is sort of dissected into parts to obtain the indices and the tag bits 
// using the division and modulo operator. It is easier to draw this in order to understand.
void calculate_location_using_address(uint32_t address,int &word_index,int& set_index,uint32_t &tag_bits,uint32_t& block_address)

{
	uint32_t word_address = (address)/(::bytes_per_word) ; //  division or Logical Shift Right.

	block_address = (word_address)/(::words_per_block) ;

	//  WE DONT CALCULATE BLOCK INDICES !

	// Extracting bits - Using a modulo is same as extracting the LSB bits 
	// since the configuration inputs ( the globals ) are always a power of 2.
	word_index = (word_address) % (::words_per_block) ;  
	set_index = (block_address) % (::sets_per_cache ) ; // NOTE - We use the block address here !
	tag_bits = (block_address)/(sets_per_cache) ;       // NOTE - We use the block address here !

}


// ====================================== READ & WRITE RESPONSE FUNCTIONS ====================================

// The wikipedia diagram - https://en.wikipedia.org/wiki/Cache_(computing)#/media/File:Write-back_with_write-allocation.svg
// provides a good overview of the workflow involved (which is discussed in more detail below )

/*  PROCEDURE -  
	    1. Check if Hit , if HIt , dont forget to increment LRU counter. - Handle both DIrect Mapped and N-way set associative.
 *          2. If MISS
 *
 *          		a) We first need to find the block to replace.
		         We either find the first invalid block ( ELSE)
		        If nothing is invalid , then all blocks are Full.
		        Hence now we apply LRU and find the block with the least LRU
		        Counter Value.
 *
 */

void Read_response(int word_index,int set_index,uint32_t tag_bits,vector< vector<block_cache> >& cache,vector<block_main_mem> &main_mem ,uint32_t block_address)
{

    int blocks_read_from_memory_counter = 0 ; // This is for the blocks that missed.
    int blocks_written_to_memory_counter =  0  ; // this is for the dirty blocks !
    int time = 0 ;

    bool hit = false ; // Important that we do this.
    string  data ;  // this is the FINAL OUTCOME.

    int hit_block_index ; // The block that we find in the loop below.

    check_hit(cache,set_index,tag_bits,hit_block_index,hit ) ;


        if( hit )
        {
           // cerr << " This is the REAd-HIT Case " << endl ;
            //if(::blocks_per_set==1 ){cout << " This is the direct mapped READ -HIT case" << endl ;}
            // It is a READ-HIT CASE.
            data = get_data( cache, word_index,set_index,hit_block_index ) ;
            ((cache[set_index])[hit_block_index]).lru_counter++ ; // Increment LRU counter of the block.
        }


    // Even after all blocks have been looped over , you still dont have a hit , then its a miss.

        else if(hit==false)
        {
                // CASE  -MISS for a N WAY SET ASSOCIATIVE CACHE.
                // STEP 1 - We Need to find the block to replace.
                //          This is done by either finding the first invalid block (OR)
                //
                    // STEP 1 - Need to find what to replace. Can be EITHER ANY Invalid block (OR) lru block
                    bool found_invalid = false ;
                    int block_with_invalid_index ;

                     for(int p=0; p< int(::blocks_per_set);p++)
                     {
                         if( ( ((cache[set_index])[p]).valid_bit==0 ) && ( ((cache[set_index])[p]).lru_counter==0 ) )  // the counter check is for extra safety.
                         {
                             found_invalid = true ;
                             block_with_invalid_index = p ;
                             break ;
                         }


                     }

                                // If its invalid , we dont care about dirty flag.
				// We replace cache contents from main memory.
				// Note - we find the block index in main memory by modulo(ing) the current block address
				//  by the number of blocks in main memory . 
                                 if(found_invalid==true)
                                 {

                                    // cout << " Replacing the invalid block(Instead of LRU ),Index" << block_with_invalid_index << endl ;
                                         // IF Replacement block is found, then HANDLE Read Miss.
                                        int new_main_mem_block_index = block_address % (::blocks_per_main_memory)  ;

                                        // The Main Task -
                                        blocks_read_from_memory_counter++ ;
                                        ((cache[set_index])[ block_with_invalid_index]).block_vector = (main_mem[new_main_mem_block_index ]).block_vector ;


                                        // Need to make sure that we "END" the READ-MISS properly.
                                        ((cache[set_index])[ block_with_invalid_index]).valid_bit = 1; // Since it has now been initialized.Safety after that.
                                        ((cache[set_index])[ block_with_invalid_index]).dirty_flag = false ; // Since it is now CONSISTENT with main_memory
                                        ((cache[set_index])[ block_with_invalid_index]).block_tag = tag_bits ;
                                        ((cache[set_index])[block_with_invalid_index]).lru_counter++ ; // DONt Forget to Increment LRU Counter.


                                        data = get_data(cache, word_index,set_index, block_with_invalid_index) ; // data - word size ( Or bytes/word )



                                 }

                                 

                                  // Now we need to hunt for the block with lowest value of LRU counter.this is least recently used block.
				// After that we check if it is dirty , if true , then we write it's content to memory.
				// Uptil here the procedure is similar to a write-miss ( if an invalid block is found )--------> Can factorize common portion into a function later.
				// At this point , it is important not to forget to copy the correct memory contents to the LRU block in the cache.
                                 else if(found_invalid==false)
                                 {
                                                // cout << " implementing LRU ...... " << endl  ;
                                                int min_counter = ((cache[set_index])[0]).lru_counter;  // We initialize it to first block.
                                                int lru_block_index = 0 ;

                                                for(int h=0;h<int(::blocks_per_set);h++)
                                                {
                                                    if( int(((cache[set_index])[h]).lru_counter) < min_counter )
                                                    {
		                                            min_counter = ((cache[set_index])[h]).lru_counter ;
		                                            lru_block_index = h ;
                                                    }
                                                }
                                                // based on our logic, the block index we get now will
                                                // be of a VALID block.However, we will still run an extra safety check for validity.
                                                // We now have the LRU block INDEX. ( and value also )
                                                // Now , We need to check if it is dirty or not.

                                                if( ( ((cache[set_index])[lru_block_index]).valid_bit == 1) && ( ((cache[set_index])[lru_block_index]).dirty_flag == 1 ) ) // Need to check for dirty flag.
                                                {
                                                     blocks_written_to_memory_counter++ ;

                                                    uint32_t dirty_dirty_byte_address = ((cache[set_index])[lru_block_index]).dirty_byte_address ;
                                                    uint32_t dirty_word_address = dirty_dirty_byte_address/(::bytes_per_word ) ;
                                                    uint32_t dirty_block_address = dirty_word_address/(::words_per_block ) ;
                                                    int new_main_mem_dirty_block_index = (dirty_block_address)%(::blocks_per_main_memory) ;

							// Update contents of memory.
                                                    (main_mem[new_main_mem_dirty_block_index]).block_vector = ((cache[set_index])[lru_block_index]).block_vector ;
                                                }

                                                // Once the dirty flag has been handled , We now need to write the stuff from memory into the lru block.

                                                int new_main_mem_block_index = block_address % (::blocks_per_main_memory)  ;

                                                // Imp- Do not forget to Copy the stuff from main memory into lru block,
                                                 blocks_read_from_memory_counter++ ;
                                                ((cache[set_index])[lru_block_index]).block_vector = (main_mem[new_main_mem_block_index ]).block_vector;

                                                // Now we need to make sure we "END" the READ-MISS correctly.
                                                 ((cache[set_index])[lru_block_index]).valid_bit = 1; // Since it has now been initialized.Safety after that.
                                                 ((cache[set_index])[lru_block_index]).dirty_flag = false ; // Since it is now CONSISTENT with main_memory
                                                 ((cache[set_index])[lru_block_index]).block_tag = tag_bits ; // Tag Update is very important.
                                                 ((cache[set_index])[lru_block_index]).lru_counter++ ; // DONt Forget to Increment LRU Counter.


                                                data = get_data( cache, word_index, set_index, lru_block_index ); // data - word size ( Or bytes/word )


                                 }


        }

    // Calculation time required (in cycles) of the read/write operations to cache and main memory
    time = calculateTime(blocks_written_to_memory_counter,blocks_read_from_memory_counter);

	std::transform(data.begin(), data.end(),data.begin(), ::toupper);

	printReadAck(hit, set_index,time,data);

   
}


/*  PROCEDURE -  
	    1. Check if Hit , if HIt , dont forget to increment LRU counter. - Handle both DIrect Mapped and N-way set associative.
		Also , no need to check if dirty flag or not. If dirty , it means the cache block is not consistent with main memory.
		But since we are going to update cache any way , no need to write the dirty content to main memory.

 *          2. If MISS

 *          	 We first need to find the block to replace.
                 We either find the first invalid block ( ELSE)
                If nothing is invalid , then all blocks are Full.
                Hence now we apply LRU and find the block with the least LRU
                Counter Value.
 *
 */

void Write_response(int word_index,int set_index,uint32_t tag_bits,vector< vector<block_cache> >& cache,vector<block_main_mem> &main_mem ,uint32_t block_address,uint32_t address,std::string data)
{
    bool hit=false;
     int blocks_read_from_memory_counter = 0 ; // This is for the blocks that missed.
    int blocks_written_to_memory_counter =  0  ; // this is for the dirty blocks !
    int time = 0 ;

    int hit_block_index ; // The block that we find in the loop below.

    check_hit(cache,set_index,tag_bits,hit_block_index,hit ) ;


	// Write- hit case.
            if( hit==true )
            {
                
               // if(::blocks_per_set==1 ){cout << " This is the direct mapped WRITE -HIT case" << endl ;}
                put_data( cache, word_index,set_index,hit_block_index,data ) ;

                ((cache[set_index])[hit_block_index]).lru_counter++ ; // Increment LRU counter of the block.
                 ((cache[set_index])[hit_block_index]).dirty_flag = true ;  // Since it is now inconsistent.
                 ((cache[set_index])[hit_block_index]).dirty_byte_address = address  ;  // THIS IS ONE EXTRA STEP ! Storing the dirty byte address
            }


            else if(hit == false)
            {
                // We now need to either - 
		// a) look for an invalid block location. This means that the block was never filled before.  
		// OR 
		// b)  the fact that we couuld not find an invalid block ,implies that  
		//	all blocks have been written to before , and hence we must use LRU replacement policy

                     
                             //cout << " this is the " << (::blocks_per_set) << " -Way Set Associative WRITE MISS Case" << endl ;
                            // STEP 1 - Need to find what to replace. Can be EITHER Invalid
                            bool found_invalid = false ;
                            int block_with_invalid_index ;

                             for(int p=0; p<int(::blocks_per_set);p++)
                             {
                                 if( ( ((cache[set_index])[p]).valid_bit==0 ) && ( ((cache[set_index])[p]).lru_counter==0 ) )  // the counter check is for extra safety.
                                 {
                                     
                                     found_invalid = true ;
                                     block_with_invalid_index = p ;
                                     break ;
                                 }


                             }

                                // NOTE - If it's an invalid block , we dont care about dirty flag.
				// We simply overwrite the cache location and mark it as dirty.
                                 if(found_invalid==true)
                                 {
                                    // cout << " Replacing the invalid block(Instead of LRU ),Index" << block_with_invalid_index << endl ;
                                         // IF Replacement block is found, then HANDLE Read Miss.
                                        int new_main_mem_block_index = block_address % (::blocks_per_main_memory)  ;

                                        // OPTIMIZATION - 1 word/block.We dont need to fetch a block from main memory
                                        //                because it will be overwritten anyway.
                                        if((::words_per_block) != 1)
                                        {
                                            // The Main Task
                                            blocks_read_from_memory_counter++  ;
                                            ((cache[set_index])[ block_with_invalid_index]).block_vector = (main_mem[new_main_mem_block_index ]).block_vector ;
                                        }


                                        // Need to make sure that we "END" the WRITE-MISS properly.
                                        ((cache[set_index])[ block_with_invalid_index]).valid_bit = 1; // Since it has now been initialized.Safety after that.
                                        ((cache[set_index])[ block_with_invalid_index]).dirty_flag = true ; // Since it is now CONSISTENT with main_memory
                                        ((cache[set_index])[ block_with_invalid_index]).block_tag = tag_bits ;
                                        ((cache[set_index])[block_with_invalid_index]).lru_counter++ ; // DONt Forget to Increment LRU Counter.
                                        ((cache[set_index])[block_with_invalid_index]).dirty_byte_address = address  ;  // THIS IS ONE EXTRA STEP ! Storing the dirty byte address


                                        put_data( cache,word_index,set_index,  block_with_invalid_index,data) ;

                                 }

                                  // Now we need to hunt for the block with lowest value of LRU counter.this is least recently used block.
				// After that we check if it is dirty , if true , then we write it's content to memory
                                 else if(found_invalid==false)
                                 {
                           
                                        int min_counter = ((cache[set_index])[0]).lru_counter;  // We initialize it to first block.
                                        int lru_block_index = 0 ;

                                        for(int h=0;h<int(::blocks_per_set);h++)
                                        {

                                            if( int(((cache[set_index])[h]).lru_counter) < min_counter )
                                            {
                                                min_counter = ((cache[set_index])[h]).lru_counter ;
                                                lru_block_index = h ;
                                            }
                                        }
                                        // based on our logic, the block index we get now will
                                        // be of a VALID block.However, we will still run an extra safety check for validity.
                                        // We now have the LRU block INDEX. ( and value also )
                                        // Now , We need to check if it is dirty or not.

                                                if( ( ((cache[set_index])[lru_block_index]).valid_bit == 1) && ( ((cache[set_index])[lru_block_index]).dirty_flag == 1 ) ) // Need to check for dirty flag.
                                                {
                                                     blocks_written_to_memory_counter++ ;

                                                    uint32_t dirty_dirty_byte_address = ((cache[set_index])[lru_block_index]).dirty_byte_address ;
                                                    uint32_t dirty_word_address = dirty_dirty_byte_address/(::bytes_per_word ) ;
                                                    uint32_t dirty_block_address = dirty_word_address/(::words_per_block ) ;
                                                    int new_main_mem_dirty_block_index = (dirty_block_address)%(::blocks_per_main_memory) ;

						   // Update main memory block with the cache block.
                                                    (main_mem[new_main_mem_dirty_block_index]).block_vector = ((cache[set_index])[lru_block_index]).block_vector ;
                                                }

                                                // Once the dirty flag has been handled , We now need to write the stuff from memory into the lru block.

                                                int new_main_mem_block_index = block_address % (::blocks_per_main_memory)  ;
	
						// Write-misses can be optimized for a 1 word
                                                // OPTIMIZATION - 1 word/block.We dont need to fetch a block from main memory
                                                //                  because it will be overwritten anyway.
                                                 if((::words_per_block) != 1)
                                                {
                                                    // Major Task - Copy the stuff from main memory into lru block,
                                                     blocks_read_from_memory_counter++ ;
                                                    ((cache[set_index])[lru_block_index]).block_vector = (main_mem[new_main_mem_block_index ]).block_vector;
                                                }

                                                // Now we need to make sure we "END" the WRITE-MISS correctly.
                                                 ((cache[set_index])[lru_block_index]).valid_bit = 1; // Since it has now been initialized.Safety after that.
                                                 ((cache[set_index])[lru_block_index]).dirty_flag = true ; // Since it is now CONSISTENT with main_memory
                                                 ((cache[set_index])[lru_block_index]).block_tag = tag_bits ; // Tag Update is very important.
                                                 ((cache[set_index])[lru_block_index]).lru_counter++ ; // DONt Forget to Increment LRU Counter.

                                                 ((cache[set_index])[lru_block_index]).dirty_byte_address = address  ;  // THIS IS ONE EXTRA STEP ! Storing the dirty byte address


                                                put_data( cache,word_index,set_index,lru_block_index,data) ;


                                 }

            }



 	// Calculation time required (in cycles) of the read/write operations to cache and main memory
	time = calculateTime(blocks_written_to_memory_counter,blocks_read_from_memory_counter);

	printWriteAck(hit, set_index,time);



}

// =============================================================================================================

// A flush causes the contents of the cache and the main memory to be same ( consistent with each other )
// We check for a valid and dirty block and copy the block to main memory
void Flush_response(vector< vector<block_cache> >& cache,vector<block_main_mem> &main_mem)
{

    int blocks_written_to_memory_counter =  0 ;

    for(int i = 0 ; i < int (::sets_per_cache) ; ++i)
    {
        std::cerr << "FLush here:" << i << std::endl ;

        for(int j = 0; j < int( ::blocks_per_set); ++j)
        {

		// If cache block is valid and dirty , then we need to update main memory.
		// Only if valid need to check for dirty flag - This is ensured by the short circuiting property of "&&". 
                if( ( ((cache[i])[j]).valid_bit == 1) && ( ((cache[i])[j]).dirty_flag == true ) ) 
                {
                        blocks_written_to_memory_counter++ ;

			// kindly excuse my variable naming sense here.
                        uint32_t dirty_dirty_byte_address = ((cache[i])[j]).dirty_byte_address ;
                        uint32_t dirty_word_address = dirty_dirty_byte_address/(::bytes_per_word ) ;
                        uint32_t dirty_block_address = dirty_word_address/(::words_per_block ) ;
                        int new_main_mem_dirty_block_index = (dirty_block_address)%(::blocks_per_main_memory) ;

			// copy cache block to main memory 
                        (main_mem[new_main_mem_dirty_block_index]).block_vector = ((cache[i])[j]).block_vector ;

			// Since the cache content was copied to main memory , the cache block 
			// no longer contains dirty data.
                        ((cache[i])[j]).dirty_flag = false ;
                }


        }
    }

    std::cout << "flush-ack " << (  blocks_written_to_memory_counter *  (::memory_write_time_in_cyces_per_block ) ) << std::endl ;

}

// ========================== Debug Response handler ==================================================

void Debug_response( const vector< vector<block_cache> > &cache)
{

    cout << "debug-ack-begin " << endl ;
    cout << endl <<  "Displaying cache contents" << endl ;

    for(int i=0;i < (::sets_per_cache) ; i++)
    {
        for(int j=0;j<(::blocks_per_set);j++)
        {
            for(int k=0;k<(::words_per_block);k++)
            {
                for(int f=0;f<(::bytes_per_word);f++)
                {
                      cout << std::hex <<  ( ((cache[i])[j]).block_vector[k][f] ) ;
                      if(f!=(::bytes_per_word-1))
                     {
                         cout << " | " ;
                     }
                }

              cout << " || " ;
            }
                if(j!=(::blocks_per_set-1))
                {cout << "  |||  "; } ;
        }


        cout << endl ; create_dashes(i) ; cout << endl ;
    }

    cout << endl <<"debug-ack-end" << endl ;
}

// a helper function
void create_dashes(int set_index )
{
    // Simple calculation based on observation to output dashes accordingly.
    int total_spaces = 4*(::bytes_per_word)*(::words_per_block)*(blocks_per_set)
                        +2*(::words_per_block)*(blocks_per_set)
                        + 2*(blocks_per_set) ;

    for(int i=0;i<total_spaces;i++)
    {
        if(i==(total_spaces/2))
        {
            cout << " Set: "<< set_index << " " ;
        }
        else
        {
            cout << "=" ;
        }

    }

}

//================================================================================================

std::string get_data( const vector< vector<block_cache> > &cache, int word_index,int set_index,int extra_index )
{
	int x ;
	stringstream ss;


	for(int k=0; k < int(::bytes_per_word);k++)
	{

		if(( (((cache[set_index])[extra_index]).block_vector[word_index])[k] )==0)
		{
			ss << "00" ; 
		}

		else
		{
			x =  ( (((cache[set_index])[extra_index]).block_vector[word_index])[k] )  ;
			ss << std::hex << x ; 
		}

	}

	std::cerr << "data that i get" <<  ss.str() ; 
	return ss.str() ;

}


// places the data as uppercase hex digits into the given location in the cache.
// the parameter "extra index" has been named simpy to avoid confusion of calling it a "block index"
// This is because we never calcyl
void put_data( vector< vector<block_cache> > &cache, int word_index,int set_index,int extra_index ,string data)
{
    std::cerr << " Data that I am putting in" << data <<  std::endl ;
    
	int x ;
	std::string temp ; 

	for( int i = 0 ; i < (::bytes_per_word) ; i++)
	{
		temp = data.substr(2*i,2) ;
		 cerr << "Temp="<<temp << endl ; 

		stringstream(temp) >> std::uppercase >> std::hex >> x ;

		cerr << "x="<<std::hex<<x << endl << endl ;
		( ((cache[set_index])[extra_index]).block_vector[word_index][i] )  = x ;
		 
	}
   
}


// since there are multiple return values ,to avoid creating structs for the return value 
//the inputs are passed by refernce.  Inefficient , replace interface later.
void check_hit(const vector< vector<block_cache> > &cache,int set_index,uint32_t tag_bits,int &hit_block_index,bool &hit )
{
    hit = false ;

    for(int i=0;i< int(::blocks_per_set);i++)
    {
         if ( ( ((cache[set_index])[i]).valid_bit == 1 ) && ( ((cache[set_index])[i]).block_tag == tag_bits ) )
         {
                 hit = true ;
                 hit_block_index = i ;
                break ; // safety.
         }
    }

}



void printReadAck(bool hit, int set_index, int time,std::string data)
{
	if(hit)
	{
		std::cout << "read-ack " << set_index << " hit " <<  time <<" " <<  std::uppercase << data <<std::nouppercase << std::dec << std::endl ;
	}

	else
	{
		std::cout << "read-ack " << set_index << " miss " <<  time << " " << std::uppercase << data <<std::nouppercase << std::dec << std::endl ;
	}    

}


void printWriteAck(bool hit, int set_index, int time)
{   

	if(hit)
	{
		std::cout << "write-ack " << set_index << " hit " <<  time  << endl ;
	}

	else
	{
		std::cout << "write-ack " << set_index << " miss " <<  time  << endl ;

	}

}

 // Calculation time required (in cycles) of the read/write operations to cache and main memory
int calculateTime(int blocks_written_to_memory_counter,int blocks_read_from_memory_counter) 
{

	int time = 0 ;

     // Calculation of the read/Write hit-time / miss-time in cycles.
     time = (::hit_time_in_cycles_per_hit)+( ( ::memory_write_time_in_cyces_per_block ) * ( blocks_written_to_memory_counter ) )
                                        + (( (::memory_read_time_in_cycles_per__block ) )*(blocks_read_from_memory_counter) ) ;
	
	return time;


}
