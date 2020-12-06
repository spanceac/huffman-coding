#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_CHARS 256
#define bit_set(byte, bit) ((byte) |= (1 << (bit)))
char str_so_far[50];
char code_of_leaf[50];
int byte_found = 0;

struct tree_node
{
	struct tree_node *left;
	struct tree_node *right;
	unsigned char leaf_byte;
};

struct code
{
	unsigned int score;
	unsigned int str_len;
	char msg[256];
	struct tree_node *subtree;
};

int was_char_evaluated(char c, struct code *my_code, int list_len)
{
	for(int i = 0; i < list_len; i++)
		if(c == my_code[i].msg[0]) return 1;
	return 0;
}

void sort_code(struct code *my_code, int code_len)
{
	struct code temp;
	while(1)
	{
		int sort = 0;
		for(int i = 0; i < code_len - 1; i++)
		{
			if(my_code[i].score > my_code[i+1].score)
			{
				sort++;
				temp = my_code[i+1];
				my_code[i+1] = my_code[i];
				my_code[i] = temp; 
			}
		}
		if(sort == 0)
			break;
	}
}

void tree_traversal(struct tree_node *head)
{
	if(head->left == NULL) /* nowhere to go left means nowhere to go right on a binary tree */
	{
		int len = strlen(str_so_far);
		if(len > 0)
		{
			printf(" %hhx: %s\n", head->leaf_byte, str_so_far);
			str_so_far[len - 1] = 0;
		}
		return;
	}
	strcat(str_so_far, "0");
	tree_traversal(head->left);
	
	strcat(str_so_far, "1");
	tree_traversal(head->right);

	int len = strlen(str_so_far);
	if(len > 0)
		str_so_far[len - 1] = 0;
	return;
}

void tree_search(struct tree_node *head, unsigned char byte)
{
	byte_found = 0;
	if(head->left == NULL) /* nowhere to go left means nowhere to go right on a binary tree */
	{
		int len = strlen(str_so_far);
		if(len > 0)
		{
			if(head->leaf_byte == byte)
			{
				byte_found = 1;
				memcpy(code_of_leaf, str_so_far, len);
			}
			str_so_far[len - 1] = 0;
		}
		return;
	}
	strcat(str_so_far, "0");
	tree_search(head->left, byte);
	if(byte_found == 1)
		return;
	strcat(str_so_far, "1");
	tree_search(head->right, byte);
	if(byte_found == 1)
		return;

	int len = strlen(str_so_far);
	if(len > 0)
		str_so_far[len - 1] = 0;
	return;
}

int main(int argc, char *argv[])
{
	FILE *f;

	if(argc < 2)
	{
		printf("Give the file to encode as parameter\n");
		return -1;
	}

	f = fopen(argv[1], "r");
	if(f == NULL)
	{
		printf("Error opening file %s\n", argv[1]);
		return -1;
	}
	struct code my_code[256] = {0};

	int eval_count = 0;
	int i = 0, c = 0;
	struct tree_node *head;

	while((c = fgetc(f)) != EOF)
	{
		if(was_char_evaluated(c, my_code, eval_count) == 0)
		{
			//evaluated[eval_count] = c;
			my_code[eval_count].msg[0] = c;
			my_code[eval_count].score++;
			my_code[eval_count].str_len = 1;
			eval_count++;
		}	
		else
		{
			for(i = 0; my_code[i].msg[0] != (char) c; i++);
			my_code[i].score++;
		}
	}

	sort_code(my_code, eval_count);

	/*for(i = 0; i < eval_count; i++)
	{
		printf("byte %hhx with score %d\n", my_code[i].msg[0], my_code[i].score);
	}*/

	int iter = eval_count;

	struct code *p_code = my_code;
	for(i = 0; i < iter - 1; i++)
	{
		p_code[1].score += p_code[0].score;
		
		struct tree_node *n_tree = malloc(sizeof(struct tree_node));	
		if(p_code[0].str_len == 1)
		{
			n_tree->left = malloc(sizeof(struct tree_node));
			n_tree->left->left = NULL;
			n_tree->left->right = NULL;
			n_tree->left->leaf_byte = p_code[0].msg[0];
		}
		else
			n_tree->left = p_code[0].subtree;

		if(p_code[1].str_len == 1)
		{
			n_tree->right = malloc(sizeof(struct tree_node));
			n_tree->right->left = NULL;
			n_tree->right->right = NULL;
			n_tree->right->leaf_byte = p_code[1].msg[0];
		}
		else
			n_tree->right = p_code[1].subtree;

		int offset = p_code[1].str_len;
		memcpy(p_code[1].msg + offset, p_code[0].msg, p_code[0].str_len);
		p_code[1].str_len += p_code[0].str_len;

		p_code[1].subtree = n_tree;
		head = p_code[1].subtree;
		p_code++;
		sort_code(p_code, iter - i - 1);
	}
	tree_traversal(head);
	//printf("Full huffman code: ");
	
	rewind(f);
	FILE *out = fopen("encoded.out", "w");
	int byte_offset = 7;
	unsigned char byte_to_write = 0;
	fputc(byte_to_write, out); // we will update the first byte later with the nr of bits from 
	while((c = fgetc(f)) != EOF)
	{
		memset(str_so_far, 0, 50);
		memset(code_of_leaf, 0, 50);
		tree_search(head, (unsigned char) c);
		int len = strlen(code_of_leaf);

		for(int i = 0; i < len; i++)
		{
			if(code_of_leaf[i] == '1')
				bit_set(byte_to_write, byte_offset);
			
			if(byte_offset == 0)
			{
				fputc(byte_to_write, out);
				byte_to_write = 0;
				byte_offset = 7;
			}
			else
				byte_offset--;
		}
	}

	if(byte_offset < 7) /* last byte not written */
	{
		fputc(byte_to_write, out);
		rewind(out);
		fputc((unsigned char) 7 - byte_offset, out); /* how many bits are valid in the last byte */
	}
	fclose(f);
	fclose(out);
	
	/* tree to file */
	byte_offset = 7;
	FILE *f_tree = fopen("tree.out", "w");
	for(int every_byte = 0; every_byte < 256; every_byte++)
	{
		memset(str_so_far, 0, 50);
		memset(code_of_leaf, 0, 50);
		byte_to_write = 0;
		byte_offset = 7;
		tree_search(head, (unsigned char) every_byte);
		int len = strlen(code_of_leaf);
		//printf("byte %x with len %d and code %s\n", (unsigned char)every_byte, len, code_of_leaf);
		fputc((char) len, f_tree); /* first byte, length in bits */
		if(len == 0) continue;
		for(int j = 0; j < len; j++)
		{
			if(code_of_leaf[j] == '1')
				bit_set(byte_to_write, byte_offset);
			
			if(byte_offset == 0)
			{
				fputc(byte_to_write, f_tree);
				byte_to_write = 0;
				byte_offset = 7;
			}
			else
				byte_offset--;
			if(j == len - 1 && byte_offset < 7)
				fputc(byte_to_write, f_tree);
		}
	}
	fclose(f_tree);
	return 0;
}
