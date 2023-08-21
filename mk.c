// STEFAN MIRUNA ANDREEA 314CA
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ALPHABET_SIZE 26
#define MAX_WORD_LENGTH 50
#define MAX_COMMAND 30
#define MAX_FILENAME 30

/* useful macro for handling error codes */
#define DIE(assertion, call_description)                                       \
	do {                                                                       \
		if (assertion) {                                                       \
			fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                 \
			perror(call_description);                                          \
			exit(errno);                                                       \
		}                                                                      \
	} while (0)

typedef struct trie_node_t trie_node_t;
struct trie_node_t {
	// the letter stored in the node
	char letter;

	/* counts how many words end with the letter stored in this node; if
	no word ends with this letter, end_of_word will be 0 */
	int end_of_word;

	/* an array with 26 pointers, each one pointing to a node containing
	a distinct letter. The point is to cover the whole English alphabet */
	trie_node_t *children[ALPHABET_SIZE];

	// the number of children of the current node
	int n_children;
};

typedef struct trie_t trie_t;
struct trie_t {
	// pointer to the root node of the trie
	trie_node_t *root;
};

// function that creates a new node and returns pointer to it
trie_node_t *create_node(char letter)
{
	trie_node_t *new_node;

	// allocate memory for the node structure
	new_node = malloc(sizeof(trie_node_t));
	// defensive programming
	DIE(!new_node, "malloc failed\n");

	// initialize all the structure fields
	new_node->letter = letter;
	new_node->end_of_word = 0;
	new_node->n_children = 0;

	// set all the children array pointers to NULL
	for (int i = 0; i < ALPHABET_SIZE; i++)
		new_node->children[i] = NULL;

	return new_node;
}

// create a trie and return a pointer to it
trie_t *create_trie(void)
{
	trie_t *trie;

	// allocate memory for the trie structure
	trie = malloc(sizeof(trie_t));
	// defensive programming
	DIE(!trie, "malloc failed\n");

	trie->root = create_node('\0');

	return trie;
}

// insert a new word in the trie
void insert_word(trie_t *trie, char word[MAX_WORD_LENGTH])
{
	trie_node_t *curr = trie->root;

	/* look for each letter of the word (from left to right) in the trie. If
	the letter already exists, look for the next one. If a letter does not
	exist yet, add a new node to the trie containing the missing letter and
	so on until the end of the word. */

	for (int i = 0; i < strlen(word); i++) {
		/* establish the relationship between the letter in itself and the
		index in the children array (convert from char to int) */
		int idx = word[i] - 'a';

		// if the letter does not exist in the trie, add it
		if (!curr->children[idx]) {
			curr->children[idx] = create_node(word[i]);
			curr->n_children++;
		}

		// go to the next letter of the word
		curr = curr->children[idx];
	}

	/* if we have reached the last letter of the word, mark it by increasing
	the counter end_of_word, which will contain the number of words ending in
	that letter */
	curr->end_of_word++;
}

// recursive function that removes a whole subtrie
void recursive_subtrie_deletion(trie_node_t *node, trie_t *trie)
{
	if (!node)
		return;

	/* go through all the children of the node and call the recursive function
	for each child (that is different from NULL) */

	for (int i = 0; i < ALPHABET_SIZE; i++) {
		/* if we found a node that does not have any children,
		get out of the loop */
		if (node->n_children == 0)
			break;

		if (node->children[i]) {
			recursive_subtrie_deletion(node->children[i], trie);
			/* after removing a child, decrement the number of children of the
			current node and make the correspondent element of the children
			array point to NULL */
			node->n_children--;
			node->children[i] = NULL;
		}
	}

	free(node);
}

// function that removes a node form the trie
void remove_word(trie_t *trie, char word[MAX_WORD_LENGTH])
{
	// start from the root
	trie_node_t *curr = trie->root;

	/* store the last parent that should NOT be deleted, because it either has
	other children depending on it or it is in itself an end of word. We should
	store the last parent node that has at least one of these special
	properties, so we can free all the nodes below it */
	trie_node_t *last_special_parent = trie->root;
	int parent_idx = (word[0] - 'a');

	for (int i = 0; i < strlen(word); i++) {
		/* establish the relationship between the letter in itself and the
		index in the children array (convert from char to int) */
		int idx = word[i] - 'a';

		/* it is enough to find only one letter of the word that does not exist
		in the trie to affirm that the node had NOT been previously inserted to
		the trie. Therefore, if the word that we are looking for does not
		exist in the tree, we must leave the function */
		if (!curr->children[idx])
			return;

		/* check if this is that kind of special node that cannot be freed,
		because it also has other nodes depending on it or stores in itself
		a word end */
		if (curr->n_children == 0 && curr->end_of_word == 0) {
			curr = curr->children[idx];
			continue;
		}

		last_special_parent = curr;
		parent_idx = idx;
		curr = curr->children[idx];
	}

	/* after finishing the previous loop, we will have a pointer to the node
	that contains the last letter of the word, so we need to change the
	indicator that lets us know if that node is a word end*/
	curr->end_of_word = 0;

	/* if the current node has other children, we cannot free it, as there
	still are other nodes depending on it */
	if (curr->n_children > 0)
		return;

	/* now that we know that the current node does not have any children, we
	should free it and all the nodes above it until we reach the last special
	parent */
	recursive_subtrie_deletion(last_special_parent->children[parent_idx], trie);

	/* decrement the children number of the parent node and make the parent
	node point to NULL */
	last_special_parent->children[parent_idx] = NULL;
	last_special_parent->n_children--;
}

// recursive function that performs autocorrect
void autocorrect(trie_t *trie, int diff, int letter_idx, trie_node_t *node,
				 char new_word[MAX_WORD_LENGTH], int *printed, int k,
				 char word[MAX_WORD_LENGTH])
{
	//check if the node is different from the trie root
	if (node != trie->root) {
		if (!node)
			return;

		/* if the number of different letters is greater than k, there is no
		point in continuing on the same route, because we have already exceeded
		the maximum of differences  */
		if (diff > k)
			return;

		/* place the letter from the current node on the following
		position in the new_word */
		new_word[letter_idx - 1] = node->letter;

		/* when the length of the new word is equal with the length of the
		original word and the current node is also an end of word, it means
		that we have found the word that we were looking for */

		if (letter_idx == strlen(word) && node->end_of_word > 0) {
			// note the fact that we have printed a word
			(*printed) = 1;

			// print the new word
			for (int i = 0 ; i < strlen(word); i++)
				printf("%c", new_word[i]);

			printf("\n");

			return;
		}
	}

	// go through the children array of the word
	for (int i = 0; i < ALPHABET_SIZE; i++) {
		// check if the pointer from the current position points to NULL
		if (!node->children[i])
			continue;

		/* if the current letter in trie is different from the current letter
		in the original word, we need to increment the counter that stores the
		number of differences */
		if (node->children[i]->letter != word[letter_idx])
			autocorrect(trie, diff + 1, letter_idx + 1, node->children[i],
						new_word, printed, k, word);
		else
			autocorrect(trie, diff, letter_idx + 1, node->children[i],
						new_word, printed, k, word);
	}
}

/* function that prepares autocorrect by initializing some variables that will
be useful when performing autocorrect */
void prepare_autocorrect(trie_t *trie, int k, char word[MAX_WORD_LENGTH])
{
	/* counter that stores the number of letters in the new word that differ
	from the original word */
	int diff = 0;

	/* indicator that tells us if we have printed something in the autocorrect
	function or not */
	int printed = 0;

	/* the letter index will be useful when forming the new word, because it
	tells us on which position to place the current letter */
	int letter_idx = 0;

	// initialize the new word with '\0'
	char new_word[MAX_WORD_LENGTH] = {0};

	// call the recursive autocorrect function
	autocorrect(trie, diff, letter_idx, trie->root, new_word, &printed,
				k, word);

	/* if we haven't printed anything in the autocorrect function, it means
	that we haven't found any suitable word, so we must print a suggestive
	message */
	if (printed == 0)
		printf("No words found\n");
}

/* recursive function that forms the first word (in lexicographical order)
starting with the given prefix */
void autocomplete_first_lexico(trie_node_t *node, trie_t *trie,
							   char first_lexico_word[MAX_WORD_LENGTH],
							   int prefix_length, int *printed,
							   trie_node_t *subtrie_root)
{
	/* check if the current node is the root of the subtrie, namely the
	node containing the last letter of the prefix */
	if (node != subtrie_root) {
		if (!node)
			return;

		/* place the current letter on the next position in the word that
		we are forming and then add the string terminator */
		first_lexico_word[prefix_length] = node->letter;
		first_lexico_word[prefix_length + 1] = '\0';

		/* check if this is the last lettter of a word stored in trie and
		if we have not printed anything yet */
		if (node->end_of_word > 0 && ((*printed) != 1)) {
			// note the fact that we have printed a word
			(*printed) = 1;

			// print the word that we have formed
			for (int i = 0 ; i < prefix_length + 1; i++)
				printf("%c", first_lexico_word[i]);

			printf("\n");
			return;
		}
	}

	/* go through the children of the current node and call the recursive
	function again for the ones that do not point to NULL */
	for (int i = 0; i < ALPHABET_SIZE; i++) {
		if (node->children[i])
			autocomplete_first_lexico(node->children[i], trie,
									  first_lexico_word, prefix_length + 1,
									  printed, subtrie_root);
	}
}

/* recursive function that forms the shortest word starting with the
given prefix */
void autocomplete_shortest(trie_node_t *subtrie_root, trie_node_t *node,
						   char min_word[MAX_WORD_LENGTH], int *min_length,
						   char prefix[MAX_WORD_LENGTH],
						   char new_word[MAX_WORD_LENGTH], int cnt)
{
	/* check if the current node is the root of the subtrie, namely the
	node containing the last letter of the prefix */
	if (node != subtrie_root) {
		if (!node)
			return;

		/* place the current letter on the next position in the word that
		we are forming and then add the string terminator */
		new_word[strlen(prefix) + cnt - 1] = node->letter;
		new_word[strlen(prefix) + cnt] = '\0';

		// check if the current node is marked as an end of word
		if (node->end_of_word > 0) {
			/* if the length of the new word is smaller than the minimum length
			or if this is the first (complete) word that we are checking (we
			have never updated the minimum length), we need to update the
			minimum length and the new word, whose length is the new minimum */

			if (cnt + strlen(prefix) < (*min_length) || (*min_length) == -1) {
				// update the minimum length
				(*min_length) = cnt + strlen(prefix);

				// form the new word (with new minimum length)
				for (int i = 0; i < cnt + strlen(prefix); i++)
					min_word[i] = new_word[i];

				/* add the basic string terminator at the end of the minimum
				length word */
				min_word[cnt + strlen(prefix)] = '\0';
			}
			return;
		}
	} else {
		/* each time we get back in the root subtrie, we need to reinitialize
		the new word, so that it will only start with the prefix*/
		for (int i = 0; i < MAX_WORD_LENGTH; i++)
			new_word[i] = '\0';

		for (int i = 0; i < strlen(prefix); i++)
			new_word[i] = prefix[i];

		new_word[strlen(prefix)] = '\0';

		cnt = 0;
	}

	/* go through the children of the current node and call the recursive
	function again for the ones that do not point to NULL */
	for (int i = 0; i < ALPHABET_SIZE; i++) {
		if (!node->children[i])
			continue;

		autocomplete_shortest(subtrie_root, node->children[i], min_word,
							  min_length, prefix, new_word, cnt + 1);
	}
}

/* recursive function that forms the most common word (with the maximum
frequency) starting with the given prefix */
void autocomplete_most_frequent(trie_node_t *subtrie_root, trie_node_t *node,
								char most_freq_word[MAX_WORD_LENGTH],
								int *max_freq, char prefix[MAX_WORD_LENGTH],
								char new_word[MAX_WORD_LENGTH], int cnt)
{
	/* check if the current node is the root of the subtrie, namely the
	node containing the last letter of the prefix */
	if (node != subtrie_root) {
		if (!node)
			return;

		/* place the current letter on the next position in the word that
		we are forming and then add the string terminator */
		new_word[strlen(prefix) + cnt - 1] = node->letter;
		new_word[strlen(prefix) + cnt] = '\0';

	} else {
		/* each time we get back in the root subtrie, we need to reinitialize
		the new word, so that it will only start with the prefix*/
		for (int i = 0; i < MAX_WORD_LENGTH; i++)
			new_word[i] = '\0';

		for (int i = 0; i < strlen(prefix); i++)
			new_word[i] = prefix[i];

		new_word[strlen(prefix)] = '\0';

		cnt = 0;
	}

// check if the current node is marked as an end of word
	if (node->end_of_word > 0) {
		if (node->end_of_word > (*max_freq) || (*max_freq) == -1) {
			// update the maximum frequency
			(*max_freq) = node->end_of_word;

			// update the most frequent word
			for (int i = 0; i < cnt + strlen(prefix); i++)
				most_freq_word[i] = new_word[i];

			// add the string terminator
			most_freq_word[cnt + strlen(prefix)] = '\0';
		}
	}

	/* go through the children of the current node and call the recursive
	function again for the ones that do not point to NULL */
	for (int i = 0; i < ALPHABET_SIZE; i++) {
		if (!node->children[i])
			continue;

		autocomplete_most_frequent(subtrie_root, node->children[i],
								   most_freq_word, max_freq, prefix, new_word,
								   cnt + 1);
	}
}

/* function that initializes the variables that will be particularly used in
the first type of autocomplete and calls the recursive function */
void prepare_autocomplete_1(char prefix[MAX_WORD_LENGTH], trie_t *trie,
							trie_node_t *curr)
{
	/* check if the subtrie root (the node that contains the last letter
	of the word) is in itself an end of word. In this case, it is also the
	first word in a lexicographical order starting with this prefix, so we
	should print it and get out of this function */
	if (curr->end_of_word > 0) {
		printf("%s\n", prefix);
		return;
	}

	/* declare and initialize an indicator that tells us if we have
	printed any word */
	int printed = 0;

	int prefix_length = strlen(prefix);

	/* initialize the string that will store the first word (in
	a lexicographical order) starting with the given prefix */
	char first_lexico_word[MAX_WORD_LENGTH] = {0};

	// put the prefix at the beginning of the string
	for (int i = 0; i < prefix_length; i++)
		first_lexico_word[i] = prefix[i];

	/* call the recursive function that will form the word
	that we are looking for */
	autocomplete_first_lexico(curr, trie, first_lexico_word, prefix_length - 1,
							  &printed, curr);

	/* if we haven't printed anything, it means that we haven't
	found any word starting with that prefix */
	if (printed == 0)
		printf("No words found\n");
}

/* function that initializes the variables that will be particularly used in
the second type of autocomplete function and calls the recursive function */
void prepare_autocomplete_2(char prefix[MAX_WORD_LENGTH], trie_t *trie,
							trie_node_t *curr)
{
	/* check if the subtrie root (the node that contains the last letter of the
	word) is in itself an end of word. In this case, it is also the shortest
	that we could find, so we should print it and get out of the function */
	if (curr->end_of_word > 0) {
		printf("%s\n", prefix);
		return;
	}

	// initialize the function calls counter
	int cnt = 0;

	// declare and initialize the word that we are trying to form with 0
	char min_word[MAX_WORD_LENGTH];
	for (int i = 0; i < MAX_WORD_LENGTH; i++)
		min_word[i] = 0;

	char new_word[MAX_WORD_LENGTH];

	// declare and initialize the minimum lentgh with -1
	int min_length = -1;

	/* call the recursive function that will form the word that we are looking
	for (the shortest one starting with the  given prefix) */
	autocomplete_shortest(curr, curr, min_word, &min_length, prefix,
						  new_word, cnt);

	/* if the minimum length has not been modified, it means that we have not
	found any word starting with that prefix */
	if (min_length == -1)
		printf("No words found\n");
	else
		printf("%s\n", min_word);
}

/* function that initializes the variables that will be particularly used in
the third type of autocomplete function and calls the recursive function */
void prepare_autocomplete_3(char prefix[MAX_WORD_LENGTH], trie_t *trie,
							trie_node_t *curr)
{
	/* declare and initialize an indicator that tells us if we have
	printed any word */
	int printed = 0;

	// declare and initialize the maximum frequency
	int max_freq = -1;

	// declare and initialize the word that we are trying to form with 0
	char most_freq_word[MAX_WORD_LENGTH];
	for (int i = 0; i < MAX_WORD_LENGTH; i++)
		most_freq_word[i] = 0;

	char new_word[MAX_WORD_LENGTH];

	// initialize the function calls counter
	int cnt = 0;

	/* call the recursive function that will form the word that we are looking
	for (the most frequent one starting with the  given prefix) */
	autocomplete_most_frequent(curr, curr, most_freq_word, &max_freq, prefix,
							   new_word, cnt);

	/* if the maximum frequency hasn't been modified, it means that we haven't
	found any word starting with the given prefix */
	if (max_freq == -1)
		printf("No words found\n");
	else
		printf("%s\n", most_freq_word);
}

/* function that is called whenever the user introduces the autocomplete
command. This function is also responsible for the redirection to a most
spcific function, according to the autocomplete parameter */
void prepare_autocomplete(char prefix[MAX_WORD_LENGTH], int crit_number,
						  trie_t *trie)
{
	trie_node_t *curr = trie->root;

	/* declare and initialize an indicator that tells us if we have
	printed any word */
	int printed;

	// indicator that tells us if we have found the prefix in the trie or not
	int prefix_not_found = 0;

	int prefix_length = strlen(prefix);

	// go through the trie, looking for the prefix given as parameter
	for (int i = 0; i < prefix_length; i++) {
		int idx = prefix[i] - 'a';
		if (!curr->children[idx]) {
			prefix_not_found = 1;
			break;
		}
		curr = curr->children[idx];
	}

	/* if we haven't found the prefix in the trie, print a suggestive
	message and get out of the function */
	if (prefix_not_found == 1) {
		printf("No words found\n");
		return;
	}

	/* identify the autocomplete command parameter and call the
	according function */

	if (crit_number == 1) {
		prepare_autocomplete_1(prefix, trie, curr);
		return;
	}

	if (crit_number == 2) {
		prepare_autocomplete_2(prefix, trie, curr);
		return;
	}

	if (crit_number == 3) {
		prepare_autocomplete_3(prefix, trie, curr);
		return;
	}

	/* if we are here, it means that the crit_number == 0,
	so we should call all the 3 functions */
	if (prefix_not_found == 1)
		printf("No words found\n");
	else
		prepare_autocomplete_1(prefix, trie, curr);

	if (prefix_not_found == 1)
		printf("No words found\n");
	else
		prepare_autocomplete_2(prefix, trie, curr);

	if (prefix_not_found == 1)
		printf("No words found\n");
	else
		prepare_autocomplete_3(prefix, trie, curr);
}

/* recursive function used to free all the nodes of a subtrie,
starting from a specific node downwards */
void recursive_free_nodes(trie_node_t *node, trie_t *trie)
{
	if (!node)
		return;

	for (int i = 0; i < ALPHABET_SIZE; i++)
		if (node->children[i])
			recursive_free_nodes(node->children[i], trie);

	free(node);
}

/* function that parses a file and inserts all the words
from the file into the trie */
void load_file(trie_t *trie, char filename[MAX_FILENAME])
{
	FILE *f = fopen(filename, "r");

	// check if the file was opened correctly
	if (!f) {
		fprintf(stderr, "Failed to open file\n");
		return;
	}

	char word[MAX_WORD_LENGTH];

	// read and insert into the trie all the words until the end of file
	while (!feof(f)) {
		fscanf(f, "%s", word);
		insert_word(trie, word);
	}

	fclose(f);
}

int main(void)
{
	char command[MAX_COMMAND];
	char word[MAX_WORD_LENGTH];
	char prefix[MAX_WORD_LENGTH];
	char filename[MAX_FILENAME];
	int crit_number;

	// read the first command introduced by the user
	scanf("%s", command);

	// create the trie that we are going to use
	trie_t *trie = create_trie();
	int k;

	/* read commands from the user and call the specific
	functions until we meet the "EXIT" command */
	while (1) {
		if (strcmp(command, "INSERT") == 0) {
			scanf("%s", word);
			insert_word(trie, word);
		} else if (strcmp(command, "REMOVE") == 0) {
			scanf("%s", word);
			remove_word(trie, word);
		} else if (strcmp(command, "AUTOCORRECT") == 0) {
			scanf("%s", word);
			scanf("%d", &k);
			prepare_autocorrect(trie, k, word);
		} else if (strcmp(command, "EXIT") == 0) {
			recursive_free_nodes(trie->root, trie);
			free(trie);
			break;
		} else if (strcmp(command, "AUTOCOMPLETE") == 0) {
			scanf("%s", prefix);
			scanf("%d", &crit_number);
			prepare_autocomplete(prefix, crit_number, trie);
		} else if (strcmp(command, "LOAD") == 0) {
			scanf("%s", filename);
			load_file(trie, filename);
		}
	scanf("%s", command);
	}

	return 0;
}

