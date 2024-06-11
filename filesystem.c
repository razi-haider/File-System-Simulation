#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 *   ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___
 *  |   |   |   |   |                       |   |
 *  | 0 | 1 | 2 | 3 |     .....             |127|
 *  |___|___|___|___|_______________________|___|
 *  |   \    <-----  data blocks ------>
 *  |     \
 *  |       \
 *  |         \
 *  |           \
 *  |             \
 *  |               \
 *  |                 \
 *  |                   \
 *  |                     \
 *  |                       \
 *  |                         \
 *  |                           \
 *  |                             \
 *  |                               \
 *  |                                 \
 *  |                                   \
 *  |                                     \
 *  |                                       \
 *  |                                         \
 *  |                                           \
 *  |     <--- super block --->                   \
 *  |______________________________________________|
 *  |               |      |      |        |       |
 *  |        free   |      |      |        |       |
 *  |       block   |inode0|inode1|   .... |inode15|
 *  |        list   |      |      |        |       |
 *  |_______________|______|______|________|_______|
 *
 *
 */

#define FILENAME_MAXLEN 8 // including the NULL char

// inode
typedef struct inode
{
    int dir; // boolean value. 1 if it's a directory.
    char name[FILENAME_MAXLEN];
    int size;         // actual file/directory size in bytes.
    int blockptrs[8]; // direct pointers to blocks containing file's content.
    int used;         // boolean value. 1 if the entry is in use.
    int rsvd;         // reserved for future use
} inode;

// directory entry
typedef struct dirent
{
    char name[FILENAME_MAXLEN];
    int namelen; // length of entry name
    int inode;   // this entry inode index
} dirent;

// Linked List 
typedef struct node
{
    struct dirent data; // Data of type 'dirent'.
    struct node *next;  // Pointer to the next node.
} node;

/**
 * @brief prints the linked list
 *
 * @param head
 */
void printList(node *head)
{
    node *ptr = head; // Pointer to traverse the linked list.
    printf("[ ");      // Print start of list indicator.
    while (ptr != NULL)
    {
        printf("%d(%s) ", ptr->data.inode, ptr->data.name); // Print inode and name.
        ptr = ptr->next; // Move to the next node.
    }
    printf("]\n"); // Print end of list indicator and newline.
}

/**
 * @brief adds a new element to the end of the linked list
 *
 * @param headaddr
 * @param inode
 * @param name
 */
void push(node **headaddr, int inode, char *name)
{
    node *link = (node *)malloc(sizeof(node)); // Allocate memory for a new node.
    link->data.inode = inode; // Set the inode value in the node.
    strcpy(link->data.name, name); // Copy the name to the node.
    link->data.namelen = strlen(name) + 1; // Calculate and set the length of the name.
    link->next = NULL; // Initialize next pointer as NULL.

    if (*headaddr == NULL)
    {
        *headaddr = link; // If list is empty, make the new node the head.
    }
    else
    {
        node *ptr = *headaddr; // Create a pointer to traverse the list.
        while (ptr->next != NULL)
        {
            ptr = ptr->next; // Move to the end of the list.
        }
        ptr->next = link; // Link the new node to the end of the list.
    }
}

/**
 * @brief deletes the element with given inode from the linked list
 *
 * @param headaddr
 * @param inode
 * @return int
 */
int delete(node **headaddr, int inode)
{
    node *current = *headaddr, *previous = NULL; // Pointers for traversal.

    while (current->data.inode != inode)
    {
        if (current->next == NULL)
        {
            printf("Inode %d not in list\n", inode); // Inode not found in list.
            return -1; // Return error code.
        }
        previous = current;
        current = current->next;
    }

    if (current == *headaddr)
    {
        *headaddr = current->next; // If the node to be deleted is the head.
    }
    else
    {
        previous->next = current->next; // Link the previous node to the next node.
    }

    free(current); // Free memory occupied by the node to be deleted.
    return 0; // Return success code.
}

/**
 * @brief returns the length of the linked list
 *
 * @param head
 * @return int
 */
int length(node *head)
{
    int length = 0; // Initialize length counter.
    node *current = head; // Pointer to traverse the list.

    while (current != NULL)
    {
        ++length; // Increment length for each node.
        current = current->next; // Move to the next node.
    }

    return length; // Return the total length of the list.
}

/**
 * @brief returns the element with given name from the linked list
 *
 * @param head
 * @param name
 * @return node*
 */
node *find(node *head, char *name)
{
    if (head == NULL)
    {
        return NULL; // If list is empty, return NULL.
    }

    node *current = head; // Pointer to traverse the list.

    while (strcmp(name, current->data.name) != 0)
    {
        if (current->next == NULL)
        {
            return NULL; // If end of list is reached, return NULL (name not found).
        }
        current = current->next; // Move to the next node.
    }

    return current; // Return pointer to the node with matching name.
}

/**
 * @brief returns the element at given index from the linked list
 *
 * @param head
 * @param index
 * @return node*
 */
node *get(node *head, int index)
{
    if (head == NULL)
    {
        return NULL; // If list is empty, return NULL.
    }

    node *current = head; // Pointer to traverse the list.

    for (int i = 0; i < index; ++i)
    {
        if (current->next == NULL)
        {
            return NULL; // If end of list is reached, return NULL (index out of bounds).
        }
        current = current->next; // Move to the next node.
    }

    return current; // Return pointer to the node at the specified index.
}

node *dataTable[127]; // Array of pointers to nodes.
inode inodeTable[16]; // Array of inodes.
int dataBitmap[127];  // Array representing a bitmap for data.

/**
 * @brief updates the file system
 *
 * @return int
 */
int update_fs() 
{
    FILE *myfs = fopen("myfs.txt", "w"); // Open a file named "myfs.txt" in write mode.

    // Loop through the inode table entries.
    for (int i = 0; i < 16; ++i)
    {
        if (inodeTable[i].used == 1) // Check if the inode entry is in use.
        {
            // Write the inode data to the file.
            fprintf(myfs, "%d %d %s %d %d %d %d %d %d %d %d %d\n", i,
                    inodeTable[i].dir, inodeTable[i].name, inodeTable[i].size,
                    inodeTable[i].blockptrs[0], inodeTable[i].blockptrs[1],
                    inodeTable[i].blockptrs[2], inodeTable[i].blockptrs[3],
                    inodeTable[i].blockptrs[4], inodeTable[i].blockptrs[5],
                    inodeTable[i].blockptrs[6], inodeTable[i].blockptrs[7]);
        }
    }

    // Write a divider line indicating the end of inode entries and the start of data entries.
    fprintf(myfs, "-1 0 data 0 0 0 0 0 0 0 0 0\n");

    node *item = NULL; // Declare a pointer to a node.
    // Loop through the inode table entries again.
    for (int i = 0; i < 16; ++i)
    {
        if (inodeTable[i].used == 1 && inodeTable[i].dir == 1) // Check if the inode entry is in use and a directory.
        {
            // Loop through the data table linked list associated with the inode.
            for (int j = 0; j < length(dataTable[inodeTable[i].blockptrs[0]]); ++j)
            {
                item = get(dataTable[inodeTable[i].blockptrs[0]], j); // Get the j-th item from the data table.
                // Write the data table entry to the file.
                fprintf(myfs, "%d %s %d\n", inodeTable[i].blockptrs[0],
                        item->data.name, item->data.inode);
            }
        }
    }

    fclose(myfs); // Close the file.
    return 0; // Return success code.
}

/**
 * @brief initializes the file system
 *
 * @return int
 */
int init_fs()
{
    FILE *myfs = fopen("myfs.txt", "r"); // Open file in read mode.
    int inode, dir, size, blockptrs[8], dataBlockIndex, flag = 1, rc = 1; // Declare variables.
    char name[FILENAME_MAXLEN]; // Array to store file names.

    if (myfs == NULL) // Check if file doesn't previously exist.
    {
        myfs = fopen("myfs.txt", "w"); // Create and open file in write mode.
        fclose(myfs); // Close file.

        // Initialize root inode.
        inodeTable[0].used = 1;
        inodeTable[0].dir = 1;
        strcpy(inodeTable[0].name, "root");
        inodeTable[0].size = 1;
        dataBitmap[0] = 1;
        push(&dataTable[0], 0, "."); // Push root directory entry.

        update_fs(); // Update the file system.
    }
    else // If file already exists.
    {
        while (rc != EOF)
        {
            if (flag == 1)
            {
                // Read and parse inode table entries.
                rc = fscanf(myfs, "%d %d %s %d %d %d %d %d %d %d %d %d", &inode,
                            &dir, name, &size, &blockptrs[0], &blockptrs[1],
                            &blockptrs[2], &blockptrs[3], &blockptrs[4],
                            &blockptrs[5], &blockptrs[6], &blockptrs[7]);
                if (inode == -1)
                {
                    flag = -1; // Set flag to indicate start of data entries.
                }
                else if (rc != EOF)
                {
                    strcpy(inodeTable[inode].name, name);
                    inodeTable[inode].dir = dir;
                    inodeTable[inode].used = 1;
                    inodeTable[inode].size = size;
                    for (int i = 0; i < size; ++i)
                    {
                        inodeTable[inode].blockptrs[i] = blockptrs[i];
                        dataBitmap[i] = 1; // Set data block as used.
                    }
                }
            }
            else
            {
                // Read and parse data table entries.
                rc = fscanf(myfs, "%d %s %d", &dataBlockIndex, name, &inode);
                if (rc != EOF)
                {
                    push(&dataTable[dataBlockIndex], inode, name); // Add data entry to the linked list.
                }
            }
        }

        fclose(myfs); // Close file.
    }
    return 0; // Return success code.
}

/**
 * @brief creates a file
 *
 * @param path
 * @param size
 * @return int
 */
int CR(char *path, int size)
{
    if (size > 8)
    {
        printf("error: Size exceeds the limit 8\n"); // Check if size exceeds limit.
        return -1; // Return error code.
    }

    int i = 0, n = 0; // Initialize variables.
    char temp[64]; // Temporary string buffer.
    strcpy(temp, path); // Copy path to temporary buffer.
    char *token = strtok(temp, "/"), arr[16][8]; // Tokenize path and create array for path components.

    // split the path by /
    while (token != NULL)
    {
        strcpy(arr[n], token); // Copy token to array.
        token = strtok(NULL, "/"); // Get next token.
        ++n; // Increment count of path components.
    }

    int currentInode = 0; // Start from root inode.
    node *item = NULL; // Node pointer.

    // traverse the given path
    for (i = 0; i < n - 1; ++i)
    {
        item = find(dataTable[inodeTable[currentInode].blockptrs[0]], arr[i]); // Find directory in path.
        if (item == NULL)
        {
            printf("error: The directory %s in the given path does not exist!\n", arr[i]); // Directory not found.
            return -1; // Return error code.
        }
        currentInode = item->data.inode; // Update current inode.
    }

    item = find(dataTable[inodeTable[currentInode].blockptrs[0]], arr[i]); // Find target file.

    // checks if target file already exists
    if (item != NULL)
    {
        printf("error: The file already exists!\n"); // File already exists.
        return -1; // Return error code.
    }

    i = 0;
    int j = 0;
    int k = 0;

    // finds an unused inode
    while (inodeTable[i].used != 0)
    {
        ++i; // Move to the next inode.
        if (i == 16)
        {
            printf("error: All inodes in use!\n"); // All inodes are in use.
            return -1; // Return error code.
        }
    }

    inodeTable[i].used = 1; // Mark inode as used.
    inodeTable[i].dir = 0; // Set inode as a file, not directory.
    strcpy(inodeTable[i].name, arr[n - 1]); // Copy name of file.
    inodeTable[i].size = size; // Set size of file.

    // finds unused data blocks
    for (k = 0; k < size; ++k)
    {
        while (dataBitmap[j] != 0)
        {
            ++j; // Move to the next data block.
            if (j == 127)
            {
                printf("error: Not enough space left!\n"); // No space left for data blocks.
                return -1; // Return error code.
            }
        }
        inodeTable[i].blockptrs[k] = j; // Assign data block to inode.
        dataBitmap[j] = 1; // Mark data block as used.
    }

    // adds file to data block of parent
    push(&dataTable[inodeTable[currentInode].blockptrs[0]], i, arr[n - 1]); // Add file to parent directory.
    update_fs(); // Update the file system.
    return 0; // Return success code.
}

/**
 * @brief deletes a file
 *
 * @param path
 * @return int
 */
int DL(char *path)
{
    int i = 0, n = 0;
    char temp[64];
    strcpy(temp, path);
    char *token = strtok(temp, "/"), arr[16][8];

    // splits the path by /
    while (token != NULL)
    {
        strcpy(arr[n], token); // Copy token to array.
        token = strtok(NULL, "/"); // Get next token.
        ++n; // Increment count of path components.
    }

    int currentInode = 0; // Start from root inode.
    node *item = NULL; // Node pointer.

    // traverse the path
    for (i = 0; i < n - 1; ++i)
    {
        item = find(dataTable[inodeTable[currentInode].blockptrs[0]], arr[i]); // Find directory in path.
        if (item == NULL)
        {
            printf("error: The directory %s in the given path does not exist!\n", arr[i]); // Directory not found.
            return -1; // Return error code.
        }
        currentInode = item->data.inode; // Update current inode.
    }
    item = find(dataTable[inodeTable[currentInode].blockptrs[0]], arr[i]); // Find target item.

    // check if target item exists and is a file
    if (item == NULL)
    {
        printf("error: The file does not exist!\n"); // File not found.
        return -1; // Return error code.
    }
    else if (inodeTable[item->data.inode].dir == 1)
    {
        printf("error: Cannot handle directories!\n"); // Cannot handle directories.
        return -1; // Return error code.
    }

    // free up data blocks used by file
    for (i = 0; i < inodeTable[item->data.inode].size; ++i)
    {
        dataBitmap[inodeTable[item->data.inode].blockptrs[i]] = 0; // Mark data block as unused.
    }

    // free up inode used by the file
    inodeTable[item->data.inode].used = 0; // Mark inode as unused.
    inodeTable[item->data.inode].size = 0; // Reset size of file.
    strcpy(inodeTable[item->data.inode].name, ""); // Clear name of file.
    delete (&dataTable[inodeTable[currentInode].blockptrs[0]],
            item->data.inode); // Delete file from parent directory.
    update_fs(); // Update the file system.

    return 0; // Return success code.
}

/**
 * @brief copies a file
 *
 * @param srcpath
 * @param dstpath
 * @return int
 */
int CP(char *srcpath, char *dstpath)
{
    int i = 0, j = 0, n = 0;
    char arr[16][8]; // Array to store path components.
    char temp[64];
    strcpy(temp, srcpath); // Copy source path to temporary buffer.
    char *token = strtok(temp, "/");

    // split the source path by /
    while (token != NULL)
    {
        if (strcmp(token, "") != 0)
        {
            strcpy(arr[n], token); // Copy token to array.
            ++n; // Increment count of path components.
        }
        token = strtok(NULL, "/");
    }
    int currentInode = 0; // Start from root inode.
    node *item = NULL; // Node pointer.

    // traverse the source path
    for (i = 0; i < n - 1; ++i)
    {
        item = find(dataTable[inodeTable[currentInode].blockptrs[0]], arr[i]); // Find directory in path.
        if (item == NULL)
        {
            printf("error: The directory %s in the given path does not exist!\n", arr[i]); // Directory not found.
            return -1; // Return error code.
        }
        currentInode = item->data.inode; // Update current inode.
    }
    item = find(dataTable[inodeTable[currentInode].blockptrs[0]], arr[n - 1]); // Find source file.

    // check if source file exists
    if (item == NULL)
    {
        printf("error: File %s not found!\n", srcpath); // Source file not found.
        return -1; // Return error code.
    }
    else if (inodeTable[item->data.inode].dir == 1)
    {
        printf("error: Cannot handle directories!\n"); // Cannot handle directories.
        return -1; // Return error code.
    }
    i = 0;
    j = 0;
    n = 0;
    for (i = 0; i < 16; ++i)
    {
        strcpy(arr[i], ""); // Clear array.
    }

    strcpy(temp, dstpath); // Copy destination path to temporary buffer.
    token = strtok(temp, "/");

    // split the destination path by /
    while (token != NULL)
    {
        if (strcmp(token, "") != 0)
        {
            strcpy(arr[n], token); // Copy token to array.
            ++n; // Increment count of path components.
        }
        token = strtok(NULL, "/");
    }
    currentInode = 0; // Start from root inode.
    node *item2 = NULL; // Node pointer.

    // traverse the destination path
    for (i = 0; i < n - 1; ++i)
    {
        item2 = find(dataTable[inodeTable[currentInode].blockptrs[0]], arr[i]); // Find directory in path.
        if (item2 == NULL)
        {
            printf("error: The directory %s in the given path does not exist!\n", arr[i]); // Directory not found.
            return -1; // Return error code.
        }
        currentInode = item2->data.inode; // Update current inode.
    }

    // check if target file already exists
    item2 = find(dataTable[inodeTable[currentInode].blockptrs[0]], arr[n - 1]);
    if (item2 != NULL)
    {
        printf("error: The file already exists!\n"); // File already exists.
        return -1; // Return error code.
    }

    i = 0;
    j = 0;
    int k = 0;

    // finds a free inode
    while (inodeTable[i].used != 0)
    {
        ++i; // Move to the next inode.
        if (i == 16)
        {
            printf("error: All inodes in use!\n"); // All inodes are in use.
            return -1; // Return error code.
        }
    }

    inodeTable[i].used = 1; // Mark inode as used.
    inodeTable[i].dir = 0; // Set inode as a file, not directory.
    strcpy(inodeTable[i].name, arr[n - 1]); // Copy name of file.
    inodeTable[i].size = inodeTable[item->data.inode].size; // Copy size of source file.

    // finds free data blocks
    for (k = 0; k < inodeTable[i].size; ++k)
    {
        while (dataBitmap[j] != 0)
        {
            ++j; // Move to the next data block.
            if (j == 127)
            {
                printf("error: Not enough space left!\n"); // No space left for data blocks.
                return -1; // Return error code.
            }
        }
        inodeTable[i].blockptrs[k] = j; // Assign data block to inode.
        dataBitmap[j] = 1; // Mark data block as used.
    }

    // add the file to parent data table
    push(&dataTable[inodeTable[currentInode].blockptrs[0]], i, arr[n - 1]); // Add file to parent directory.
    update_fs(); // Update the file system.
    return 0; // Return success code.
}

/**
 * @brief moves a file
 *
 * @param srcpath
 * @param dstpath
 * @return int
 */
int MV(char *srcpath, char *dstpath)
{
    int i = 0, n = 0;
    char arr[16][8];
    char temp[64];
    strcpy(temp, srcpath);
    char *token = strtok(temp, "/");

    // split source path by /
    while (token != NULL)
    {
        if (strcmp(token, "") != 0)
        {
            strcpy(arr[n], token); // Copy token to array.
            ++n; // Increment count of path components.
        }
        token = strtok(NULL, "/");
    }
    int currentInode = 0; // Start from root inode.
    node *item = NULL; // Node pointer.

    // traverse source path
    for (i = 0; i < n - 1; ++i)
    {
        item = find(dataTable[inodeTable[currentInode].blockptrs[0]], arr[i]); // Find directory in path.
        if (item == NULL)
        {
            printf("error: The directory %s in the given path does not exist!\n", arr[i]); // Directory not found.
            return -1; // Return error code.
        }
        currentInode = item->data.inode; // Update current inode.
    }
    item = find(dataTable[inodeTable[currentInode].blockptrs[0]], arr[n - 1]); // Find source file.

    // check if source file exists
    if (item == NULL)
    {
        printf("error: File %s does not exist!\n", srcpath); // Source file not found.
        return -1; // Return error code.
    }
    else if (inodeTable[item->data.inode].dir == 1)
    {
        printf("error: Cannot handle directories!\n"); // Cannot handle directories.
        return -1; // Return error code.
    }
    i = 0;
    n = 0;
    int dataTableSrc = inodeTable[currentInode].blockptrs[0];
    for (i = 0; i < 16; ++i)
    {
        strcpy(arr[i], ""); // Clear array.
    }
    strcpy(temp, dstpath); // Copy destination path to temporary buffer.
    token = strtok(temp, "/");

    // split the destination path by /
    while (token != NULL)
    {
        if (strcmp(token, "") != 0)
        {
            strcpy(arr[n], token); // Copy token to array.
            ++n; // Increment count of path components.
        }
        token = strtok(NULL, "/");
    }
    currentInode = 0; // Start from root inode.
    node *item2 = NULL; // Node pointer.

    // traverse the destination path
    for (i = 0; i < n - 1; ++i)
    {
        item2 = find(dataTable[inodeTable[currentInode].blockptrs[0]], arr[i]); // Find directory in path.
        if (item2 == NULL)
        {
            printf("error: The directory %s in the given path does not exist!\n", arr[i]); // Directory not found.
            return -1; // Return error code.
        }
        currentInode = item2->data.inode; // Update current inode.
    }
    item2 = find(dataTable[inodeTable[currentInode].blockptrs[0]], item->data.name); // Find target item.

    // checks if destination file already exists
    if (item2 != NULL)
    {
        printf("error: The file already exists!\n"); // File already exists.
        return -1; // Return error code.
    }

    // update the inode for existing file
    push(&dataTable[inodeTable[currentInode].blockptrs[0]], item->data.inode, arr[n - 1]); // Add file to destination directory.
    strcpy(inodeTable[item->data.inode].name, arr[n - 1]); // Update file name.
    delete (&dataTable[dataTableSrc], item->data.inode); // Delete file from source directory.
    update_fs(); // Update the file system.
    return 0; // Return success code.
}

/**
 * @brief creates a directory
 *
 * @param path
 * @return int
 */
int CD(char *path)
{
    int i = 0, n = 0;
    char arr[16][8];
    char temp[64];
    strcpy(temp, path);
    char *token = strtok(temp, "/");

    // Split the path by /
    while (token != NULL)
    {
        strcpy(arr[n], token); // Copy token to array.
        token = strtok(NULL, "/");
        ++n; // Increment count of path components.
    }
    int currentInode = 0; // Start from root inode.
    node *item = NULL; // Node pointer.

    // Traverse the path
    for (i = 0; i < n - 1; ++i)
    {
        item = find(dataTable[inodeTable[currentInode].blockptrs[0]], arr[i]); // Find directory in path.
        if (item == NULL)
        {
            printf("error: %s not in directory %s!\n", arr[i], inodeTable[currentInode].name); // Directory not found in current directory.
            return -1; // Return error code.
        }
        currentInode = item->data.inode; // Update current inode.
    }
    item = find(dataTable[inodeTable[currentInode].blockptrs[0]], arr[n - 1]); // Find target directory.

    // Check if the target directory already exists
    if (item != NULL)
    {
        printf("error: Directory already exists!\n"); // Directory already exists.
        return -1; // Return error code.
    }

    i = 0;
    int j = 0;

    // Find an unused inode
    while (inodeTable[i].used != 0)
    {
        ++i; // Move to next inode.
        if (i == 16)
        {
            printf("error: All inodes in use!\n"); // All inodes are in use.
            return -1; // Return error code.
        }
    }
    inodeTable[i].used = 1;
    inodeTable[i].dir = 1;
    strcpy(inodeTable[i].name, arr[n - 1]); // Copy directory name to inode.
    inodeTable[i].size = 1;

    // Find unused data block
    while (dataBitmap[j] != 0)
    {
        ++j; // Move to next data block.
        if (j == 127)
        {
            printf("error: Not enough space left!\n"); // No available data blocks.
            return -1; // Return error code.
        }
    }

    // Set inode and data table
    inodeTable[i].blockptrs[0] = j; // Set block pointer.
    dataBitmap[j] = 1; // Mark data block as used.
    push(&dataTable[j], i, "."); // Add '.' entry to data block.
    push(&dataTable[j], currentInode, ".."); // Add '..' entry to data block.
    push(&dataTable[inodeTable[currentInode].blockptrs[0]], i, arr[n - 1]); // Add directory to parent data block.
    update_fs(); // Update the file system.

    return 0; // Return success code.
}

/**
 * @brief deletes a directory
 *
 * @param path
 * @return int
 */
int DD(char *path)
{
    // Initialize variables
    int i = 0, n = 0;
    char arr[16][8]; // Array to store split path components
    char temp[64];
    strcpy(temp, path);
    char *token = strtok(temp, "/"); // Tokenize the path

    // Split the path by /
    while (token != NULL)
    {
        if (strcmp(token, "") != 0) // Check if token is not empty
        {
            strcpy(arr[n], token); // Copy token to arr
            ++n;
        }
        token = strtok(NULL, "/"); // Get next token
    }

    // Check if trying to delete root directory
    if (n == 0)
    {
        printf("error: Cannot delete root directory!\n");
        return -1;
    }

    int currentInode = 0; // Initialize with root inode
    node *item = NULL;

    // Traverse the given path
    for (i = 0; i < n - 1; ++i)
    {
        item = find(dataTable[inodeTable[currentInode].blockptrs[0]], arr[i]);
        if (item == NULL)
        {
            printf(
                "error: The directory %s in the given path does not exist!\n",
                arr[i]);
            return -1;
        }
        else
        {
            currentInode = item->data.inode; // Update current inode
        }
    }

    item = find(dataTable[inodeTable[currentInode].blockptrs[0]], arr[n - 1]);

    // Check if target directory exists
    if (item == NULL)
    {
        printf("error: The directory does not exist!\n");
        return -1;
    }
    else if (inodeTable[item->data.inode].dir == 0)
    {
        printf("error: Cannot handle files!\n");
    }
    else
    {
        node *tempNode;
        char childPath[64];
        currentInode = item->data.inode;
        int parentInode =
            find(dataTable[inodeTable[currentInode].blockptrs[0]], "..")
                ->data.inode;

        // Loop through items in directory
        while (length(dataTable[inodeTable[currentInode].blockptrs[0]]) > 0)
        {
            tempNode = get(dataTable[inodeTable[currentInode].blockptrs[0]], 0);
            if (strcmp(tempNode->data.name, ".") != 0 &&
                strcmp(tempNode->data.name, "..") != 0)
            {
                // Recursive call for sub directories
                if (inodeTable[tempNode->data.inode].dir == 1)
                {
                    strcpy(childPath, path);
                    strcat(childPath, "/");
                    strcat(childPath, tempNode->data.name);
                    DD(childPath);
                }

                // Delete files inside directory
                else
                {
                    for (i = 0; i < inodeTable[tempNode->data.inode].size; ++i)
                    {
                        dataBitmap[inodeTable[tempNode->data.inode]
                                       .blockptrs[i]] = 0;
                    }
                    inodeTable[tempNode->data.inode].used = 0;
                    inodeTable[tempNode->data.inode].size = 0;
                    strcpy(inodeTable[tempNode->data.inode].name, "");
                    delete (&dataTable[inodeTable[currentInode].blockptrs[0]],
                            tempNode->data.inode);
                }
            }
            else
            {
                // Delete . and .. from data table
                delete (&dataTable[inodeTable[currentInode].blockptrs[0]],
                        tempNode->data.inode);
            }
        }

        // Delete inode of directory
        delete (&dataTable[inodeTable[parentInode].blockptrs[0]], currentInode);
        dataBitmap[inodeTable[currentInode].blockptrs[0]] = 0;
        inodeTable[currentInode].used = 0;
        inodeTable[currentInode].size = 0;
        strcpy(inodeTable[currentInode].name, "");
        update_fs(); // Update the file system.
        return 0; // Return success code.
    }
    return 0; // Return success code.
}

/**
 * @brief lists files and directories
 *
 * @param path
 * @return int
 */
int LL(char *path)
{
    // Initialize variables
    int i = 0, n = 0;
    char arr[16][8]; // Array to store split path components
    char temp[64];
    strcpy(temp, path);
    char *token = strtok(temp, "/"); // Tokenize the path

    // Split the path by /
    while (token != NULL)
    {
        if (strcmp(token, "") != 0) // Check if token is not empty
        {
            strcpy(arr[n], token); // Copy token to arr
            ++n;
        }
        token = strtok(NULL, "/"); // Get next token
    }

    int currentInode = 0; // Initialize with root inode
    node *item = NULL;
    int size = 0;

    // Traverse the path
    for (i = 0; i < n - 1; ++i)
    {
        item = find(dataTable[inodeTable[currentInode].blockptrs[0]], arr[i]);
        if (item == NULL)
        {
            printf(
                "error: The directory %s in the given path does not exist!\n",
                arr[i]);
            return -1;
        }
        currentInode = item->data.inode; // Update current inode
    }

    // Handle root directory
    if (n == 0)
    {
        currentInode = 0;
    }
    else
    {
        currentInode =
            find(dataTable[inodeTable[currentInode].blockptrs[0]], arr[n - 1])
                ->data.inode;
    }
    char childPath[64];

    // Loop through items in directory
    for (i = 0; i < length(dataTable[inodeTable[currentInode].blockptrs[0]]);
         ++i)
    {
        item = get(dataTable[inodeTable[currentInode].blockptrs[0]], i);
        if (strcmp(item->data.name, ".") != 0 &&
            strcmp(item->data.name, "..") != 0)
        {
            strcpy(childPath, strcmp(path, "/") == 0 ? "" : path);
            strcat(childPath, "/");
            strcat(childPath, item->data.name);

            // Recursive call for subdirectories
            if (inodeTable[item->data.inode].dir == 1)
            {
                size += LL(childPath);
            }
            else
            {
                printf("type: file\npath: %s\nsize: %d\n\n", childPath,
                       inodeTable[item->data.inode].size);
                size += inodeTable[item->data.inode].size;
            }
        }
    }
    ++size; // Add size of directory
    printf("type: directory\npath: %s\nsize: %d\n\n", path, size);
    return size;
}

/**
 * @brief main function
 *
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, char *argv[])
{
    // Check if the number of arguments is correct
    if (argc != 2)
    {
        printf("error: Invalid number of arguments!\n");
        return -1;
    }

    // Open the input file
    FILE *inpFile = fopen(argv[1], "r");

    // Initialize variables
    char line[64], inpCommand[3][64], *token = NULL;
    int i;

    // Initialize the file system
    init_fs();

    // Read commands from the input file
    while (fgets(line, sizeof(line), inpFile))
    {
        // Remove newline character from the end of the line
        if (line[strlen(line) - 1] == '\n')
        {
            line[strlen(line) - 1] = '\0';
        }

        i = 0;
        token = strtok(line, " ");

        // Split the input command by " "
        while (token != NULL)
        {
            strcpy(inpCommand[i], token);
            token = strtok(NULL, " ");
            ++i;
        }

        // Execute the appropriate command based on the input
        if (strcmp(inpCommand[0], "CR") == 0)
        {
            // File create
            CR(inpCommand[1], atoi(inpCommand[2]));
        }
        else if (strcmp(inpCommand[0], "DL") == 0)
        {
            // File delete
            DL(inpCommand[1]);
        }
        else if (strcmp(inpCommand[0], "CP") == 0)
        {
            // File copy
            CP(inpCommand[1], inpCommand[2]);
        }
        else if (strcmp(inpCommand[0], "MV") == 0)
        {
            // File move
            MV(inpCommand[1], inpCommand[2]);
        }
        else if (strcmp(inpCommand[0], "CD") == 0)
        {
            // Create directory
            CD(inpCommand[1]);
        }
        else if (strcmp(inpCommand[0], "DD") == 0)
        {
            // Delete directory
            DD(inpCommand[1]);
        }
        else if (strcmp(inpCommand[0], "LL") == 0)
        {
            // List files and directories
            LL("/");
        }
    }

    // Close the input file
    fclose(inpFile);

    return 0; //Return success code.
}
