#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_CHARS 256
#define bit_set(byte, bit) ((byte) |= (1 << (bit)))
#define bit_test(byte, bit) (((byte) >> (bit)) & 1)

//#define DEBUG

char str_so_far[50];
char code_of_leaf[50];
int byte_found = 0;
int bits_searched = 0;
int current_bit = 15;

struct tree_node
{
	struct tree_node *left;
	struct tree_node *right;
	unsigned char leaf_byte;
};

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

int code_to_tree_value(struct tree_node *head, unsigned char code, int *bit_offset)
{
	static struct tree_node *local = NULL;
	if(local == NULL)
		local = head;
	for(; *bit_offset >= 0; (*bit_offset)--)
	{
		if(bit_test(code, *bit_offset) == 0)
		{
			local = local->left;
		}
		else
		{
			local = local->right;
		}
		if(local->left == NULL) /* this is the leaf node */
		{
			(*bit_offset)--;
			unsigned char leaf_val = local->leaf_byte;
			local = NULL;
			return leaf_val;
		}
	}
	return -1;
}

void tree_leaf_add(struct tree_node *head, unsigned short code, int code_len, unsigned char leaf_val)
{
#ifdef DEBUG
	printf("code is %hx, code_len is %d \n", code, code_len);
#endif

	for(int i = 15; 15 - i  < code_len; i--)
	{
		if(head->left == NULL)
		{
			head->left = malloc(sizeof(struct tree_node));
			head->right = malloc(sizeof(struct tree_node));
			head->left->left = NULL;
			head->left->right = NULL;
			head->right->left = NULL;
			head->right->right = NULL;
		}
		if(bit_test(code, i) == 0)
		{
			//printf("0");
			/* go left */
			head = head->left;
		}
		else
		{
			//printf("1");
			/* go right */
			head = head->right;
		}
		if(15 - i == code_len - 1) /* this is the leaf node */
		{
			head->leaf_byte = leaf_val;
			//printf(" for %hhx \n", head->leaf_byte);
		}
	}
}

int main(int argc, char *argv[])
{
	FILE *f_tree, *f_encoded, *f_decoded;

	if(argc < 3)
	{
		printf("Pass the Huffman tree file as first parameter and the\
 encoded file as the second\n");
		return -1;
	}
	f_tree = fopen(argv[1], "r");
	if(f_tree == NULL)
	{
		printf("Error opening file %s\n", argv[1]);
		return -1;
	}

	struct tree_node *head = malloc(sizeof(struct tree_node));
	head->left = NULL;
	head->right = NULL;

	for(int each_byte = 0; each_byte < 256; each_byte++) 
	{
		int len = fgetc(f_tree);
		unsigned short code = 0;
		unsigned char byte1 = 0, byte2 = 0;
		if(len == 0) continue;
		byte1 = fgetc(f_tree);
		code = byte1 << 8;
		if(len > 8)
		{
			byte2 = fgetc(f_tree);
			code |= byte2;
		}
		tree_leaf_add(head, code, len, (unsigned char) each_byte);
#ifdef DEBUG
		printf("adding byte %d, with len %d, and code %x\n", each_byte, len, code);
#endif
	}

	fclose(f_tree);
	memset(str_so_far, 0, 50);
#ifdef DEBUG
	tree_traversal(head);
#endif

	f_encoded = fopen(argv[2], "r");
	if(f_encoded == NULL)
	{
		printf("Error opening file %s\n", argv[2]);
		return -1;
	}
	f_decoded = fopen("decoded.out", "w");
	int c = 0;
	int last_byte_valid_bits = fgetc(f_encoded);

	while(1)
	{
		int bit_offset = 7;
		int ret = 0;
		c = fgetc(f_encoded);

		if(c == EOF) break;
		else
		{
			if(fgetc(f_encoded) == EOF)
			{
				if(last_byte_valid_bits > 0)
				{
					c = c >> (8 - last_byte_valid_bits);
					bit_offset = last_byte_valid_bits - 1;
				}
			}
			else /* file pos not incremented if EOF */
				fseek(f_encoded, ftell(f_encoded) - 1, SEEK_SET);
		}
		while(1)
		{
			ret = code_to_tree_value(head, (unsigned char) c, &bit_offset);
			if(ret >= 0)
			{
				fputc((unsigned char) ret, f_decoded);
				
				if(bit_offset == -1) /* end of byte and code found */
					break;
			}
			else break; /* end of byte without finding the code */
		}
	}
	fclose(f_encoded);
	fclose(f_decoded);
	return 0;
}
