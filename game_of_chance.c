#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>
#include "hacking.h"

#define DATAFILE "/var/chance.data"               //file to store user data

//custom user struct to store information about users
struct user {
  int uid;
  int credits;
  int highscore;
  char name[100];
  int (*current_game) ();
};

//function prototypes
int get_player_data();
void register_new_player();
void update_player_data();
void show_highscore();
void jackpot();
void input_name();
void print_cards(char *, char *, int);
int take_wager(int, int);
void play_the_game();
int pick_a_number();
int dealer_no_match();
int find_the_ace();
void fatal(char *);

//global variables
struct user player;                             //player struct

int main() {
  int choice, last_game;

  srand(time(0));                               //seed the randomizer with current time

  if(get_player_data() == -1)                   //try to read player data from file
    register_new_player();

  while(choice != 7) {
    printf("-=[ Game of Chance Menu ]=-\n");
    printf("1 - Play the Pick a Number game\n");
    printf("2 - Play the No Match Dealer game\n");
    printf("3 - Play the Fina the Ace game\n");
    printf("4 - View current high score\n");
    printf("5 - Change your user name\n");
    printf("6 - Reset your account at 100 credits\n");
    printf("7 - Quit\n");
    printf("[Name: %s]\n", player.name);
    printf("[You have %u credits] -> ", player.credits);
    scanf("%d", &choice);

    if((choice < 1) || (choice > 7))
      printf("\n[!!] The number %d is an invalid selection.\n\n", choice);
    else if (choice < 4) {
      if(choice != last_game) {
        if(choice == 1)
          player.current_game = pick_a_number;
        else if(choice == 2)
          player.current_game = dealer_no_match;
        else
          player.current_game = find_the_ace;
        last_game = choice;
      }
      play_the_game();
    }
    else if(choice == 4)
      show_highscore();
    else if(choice == 5) {
      printf("\nChange user name\n");
      printf("Enter your new name: ");
      input_name();
      printf("Your name has been changed.\n\n");
    }
    else if(choice == 6) {
      printf("\nYour account has been reset with 100 credits.\n\n");
      player.credits = 100;
    }
  }
  update_player_data();
  printf("\nThanks for playing\n");
}

//this function reads the player data for the current uid
//from the file it returns -1 if unable to find the player data for current uid
int get_player_data() {
  int fd, uid, read_bytes;
  struct user entry;

  uid = getuid();

  fd = open(DATAFILE, O_RDONLY);
  if(fd == -1)
    return -1;
  read_bytes = read(fd, &entry, sizeof(struct user));
  while(entry.uid != uid && read_bytes > 0) {
    read_bytes = read(fd, &entry, sizeof(struct user));
  }
  close(fd);
  if(read_bytes < sizeof(struct user))
    return -1;
  else
    player = entry;
  return 1;
}

//this is new user registration function
//it will create a new player account and append it to the file
void register_new_player() {
  int fd;

  printf("-=-={ New Player Registration }=-=-\n");
  printf("Enter your name: ");
  input_name();

  player.uid = getuid();
  player.highscore = player.credits = 100;

  fd = open(DATAFILE, O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
  if(fd == -1)
    fatal("in register_new_player() while opening file");
  write(fd, &player, sizeof(struct user));
  close(fd);

  printf("\nWelcome to the Game of Chance %s.\n", player.name);
  printf("You have been given %u credits.\n", player.credits);
}

//This function writes the current player data to the file
//it is used primarily for updating the credits after games
void update_player_data() {
  int fd, i, read_uid;
  char burned_byte;

  fd = open(DATAFILE, O_RDWR);
  if(fd == -1)
    fatal("in update_player_data() while opening file");
  read(fd, &read_uid, 4);
  while(read_uid != player.uid) {
    for(i = 0; i < sizeof(struct user) - 4; i ++)
      read(fd, &burned_byte, 1);
    read(fd, &read_uid, 4);
  }
  write(fd, &(player.credits), 4);
  write(fd, &(player.highscore), 4);
  write(fd, &(player.name), 100);
  close(fd);
}

//this function will display the current high score and name of user who set it
void show_highscore() {
  unsigned int top_score = 0;
  char top_name[100];
  struct user entry;
  int fd;

  printf("\n===============| HIGH SCORE |===============\n");
  fd = open(DATAFILE, O_RDONLY);
  if(fd == -1)
    fatal("in show_highscore() while opening file");
  while(read(fd, &entry, sizeof(struct user)) > 0) {
    if(entry.highscore > top_score) {
      top_score = entry.highscore;
      strcpy(top_name, entry.name);
    }
  }
  close(fd);
  if(top_score > player.highscore)
    printf("%s has the high score of %u\n", top_name, top_score);
  else
    printf("You currently have the high score of %u credits\n", player.highscore);
  printf("=============================================\n\n");
}

//this function simply awards jackpot for pick a number game
void jackpot() {
  printf("*+*+*+*+*+*+ JACKPOT +*+*+*+*+*+*+*+*\n");
  printf("You have won the jackpot of 100 credits\n");
  player.credits += 100;
}

//this function is used to input the player name since scanf will stop input at first space
void input_name() {
  char *name_ptr, input_char='\n';
  while(input_char == '\n')
    scanf("%c", &input_char);

  name_ptr = (char *) &(player.name);
  while(input_char != '\n') {
    *name_ptr = input_char;
    scanf("%c", &input_char);
    name_ptr++;
  }
  *name_ptr = 0;
}

//this function prints the 3 cards for the find the ace game
void print_cards(char * message, char *cards, int user_pick) {
  int i;

  printf("\n\t*** %s ***\n", message);
  printf("      \t._.\t.-.\t.-.\n");
  printf("Cards:\t|%c|\t|%c|\t|%c|\n\t", cards[0], cards[1], cards[2]);
  if(user_pick == -1)
    printf(" 1 \t 2 \t 3\n");
  else {
    for(i = 0; i < user_pick; i++)
      printf("\t");
    printf(" ^-- your pick\n");
  }
}

//this function inputs wagers for both no match dealer and find the ace games
int take_wager(int available_credits, int previous_wager) {
  int wager, total_wager;

  printf("How many of your %d credits would you like to wager?  ", available_credits);
  scanf("%d", &wager);
  if(wager < 1) {
    printf("Not a proper wager\n");
    return -1;
  }
  total_wager = previous_wager + wager;
  if(total_wager > available_credits) {
    printf("Your total wager of %d is more than you have\n", total_wager);
    printf("You only have %d available credits, try again\n", available_credits);
    return -1;
  }
  return wager;
}

//this function contains a loop to allow the current game to be played again
void play_the_game() {
  int play_again = 1;
  int (*game) ();
  char selection;

  while(play_again) {
    printf("\n[DEBUG] current_game pointer @ 0x%08x\n", player.current_game);
    if(player.current_game() != -1) {
      if(player.credits > player.highscore)
        player.highscore = player.credits;
      printf("\nYou now have %u credits\n", player.credits);
      update_player_data();
      printf("Would you like to play again? (y/n)  ");
      selection = '\n';
      while(selection == '\n')
        scanf("%c", &selection);
      if(selection == 'n')
        play_again = 0;
    }
    else
      play_again = 0;
  }
}

//this function is the pick a number game
int pick_a_number() {
  int pick, winning_number;

  printf("\n###### Pick a Number ######\n");
  printf("This Game costs 10 credits to play. Pick a Number\n");
  printf("between 1 and 20, and if you pick the winning number you win jackpot of 100 credits\n\n");
  winning_number = (rand() % 20) + 1;
  if(player.credits < 10) {
    printf("You only have %d credits. Thats not enough to play\n\n", player.credits);
    return -1;
  }
  player.credits -= 10;
  printf("10 credits have been deducted from you account\n");
  printf("Pick a number between 1 and 20:  ");
  scanf("%d", &pick);

  printf("the winning number is %d\n", winning_number);
  if(pick == winning_number)
    jackpot();
  else
    printf("You Didn't win\n");
  return 0;
}

//This is no match dealer game
int dealer_no_match() {
  int i, j, numbers[16], wager = -1, match = -1;

  printf("\n:::::: No Match Dealer ::::::\n");
  printf("You can wager up to all of your credits\n");
  printf("the dealer will deal out 16 random numbers between 0 and 99.\n");
  printf("If There are no matches among them, you double your money\n\n");

  if(player.credits == 0) {
    printf("You dont have any credits to wager\n");
    return -1;
  }
  while(wager == -1)
    wager = take_wager(player.credits, 0);

  printf("\t\t::: Dealing out 16 random numbers :::\n");
  for(i = 0; i < 16; i++) {
    numbers[i] = rand() % 100;
    printf("%2d\t", numbers[i]);
    if(i%8 == 7)
      printf("\n");
  }
  for(i = 0; i < 15; i ++) {
    j = i + 1;
    while(j < 16) {
      if(numbers[i] == numbers[j])
        match = numbers[i];
      j++;
    }
  }
  if(match != -1) {
    printf("The dealer matched the number %d\n", match);
    printf("You lose %d credits\n", wager);
    player.credits -= wager;
  }
  else {
    printf("there were no matches, you win %d credits\n", wager);
    player.credits += wager;
  }
  return 0;
}

//This is the find the ace game
int find_the_ace() {
  int i, ace, total_wager;
  int invalid_choice, pick = -1, wager_one = -1, wager_two = -1;
  char choice_two, cards[3] = {'X', 'X', 'X'};

  ace = rand()%3;

  printf("****** Find the ACE ******\n");
  printf("In this game you can wager up to all of your credits\n");
  printf("three cards will be dealt out, two queens and one ace\n");
  printf("if you find the ace, you will win your wager\n");
  printf("after choosing a card, one of the queens will be revealed\n");
  printf("at this point, you may either select a different card or increase your wager\n\n");

  if(player.credits == 0) {
    printf("you dont have enough credits\n\n");
    return -1;
  }

  while(wager_one == -1)
    wager_one = take_wager(player.credits, 0);

  print_cards("dealing cards", cards, -1);
  pick = -1;
  while((pick < 1) || (pick > 3)) {
    printf("select a card: 1, 2, or 3 ");
    scanf("%d", &pick);
  }
  pick--;
  i = 0;
  while(i == ace || i == pick) {
    i++;
  }
  cards[i] = 'Q';
  print_cards("Revealing a queen", cards, pick);
  invalid_choice = 1;
  while(invalid_choice) {
    printf("would you like to:\n[c]hange your pick\tor\t[i]ncrese your wager?\n");
    printf("Select c or i;  ");
    choice_two = '\n';
    while(choice_two == '\n')
      scanf("%c", &choice_two);
    if(choice_two == 'i') {
      invalid_choice = 0;
      while(wager_two == -1)
       wager_two =  take_wager(player.credits, wager_one);
      }
    if(choice_two == 'c') {
      i = invalid_choice = 0;
      while(i == pick || cards[i] == 'Q')
        i ++;
      pick = i;
      printf("Your card pick has been changed to card %d\n", pick + 1);
    }
  }

  for(i = 0; i < 3; i++) {
    if(ace == i)
      cards[i] = 'A';
    else
      cards[i] = 'Q';
  }
  print_cards("End Result", cards, pick);

  if(pick == ace) {
    printf("You have won %d credits from your first wager\n", wager_one);
    player.credits += wager_one;
    if(wager_two != -1) {
      printf("and an additional %d credits from your second wager\n", wager_two);
      player.credits += wager_two;
    }
  } else {
    printf("you have lost %d credits from your first wager\n", wager_one);
    player.credits -= wager_one;
    if(wager_two != -1) {
      printf("and an additional %d credits from your second wager\n", wager_two);
      player.credits -= wager_two;
    }
  }
  return 0;
}





