/*
 * Unstructured Dynamic Database Management System (NoSQL-like) in C
 * -----------------------------------------------------------------
 * Features:
 * - UNSTRUCTURED STORAGE: Records (Documents) have no fixed schema.
 * - User defines keys and values at runtime (e.g., Name: John, Age: 20).
 * - Fully dynamic memory allocation (structs, arrays, and strings).
 * - Mimics a Document Store (like MongoDB).
   * ---------------------------------------
  * Author:
  * - Shubhagaman Singh
  * - https://shubhagaman-74c28.web.app
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 256

// A single piece of data (Key-Value pair)
typedef struct {
    char *key;
    char *value;
} Field;

// A Document: A collection of dynamic fields
typedef struct {
    Field *fields;      // Dynamic array of fields
    int field_count;    // How many fields in this document
} Document;

// Global Database
Document *db = NULL;
int doc_count = 0;

// Function Prototypes
void create_document();
void read_documents();
void update_document();
void delete_document();
void search_documents();
void free_all_memory();
void clear_input_buffer();
char* get_dynamic_string(const char* prompt);

int main() {
    int choice;

    printf("=== Unstructured (NoSQL) Dynamic DBMS ===\n");
    printf("Store any data. No fixed columns. Fully dynamic.\n");

    while (1) {
        printf("\n--- Main Menu ---\n");
        printf("1. Create Document (Add Record)\n");
        printf("2. Read All Documents\n");
        printf("3. Search Documents (by Key-Value)\n");
        printf("4. Update Document\n");
        printf("5. Delete Document\n");
        printf("6. Exit\n");
        printf("Enter choice: ");
        
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input.\n");
            clear_input_buffer();
            continue;
        }
        clear_input_buffer();

        switch (choice) {
            case 1: create_document(); break;
            case 2: read_documents(); break;
            case 3: search_documents(); break;
            case 4: update_document(); break;
            case 5: delete_document(); break;
            case 6:
                free_all_memory();
                printf("Database flushed. Exiting.\n");
                exit(0);
            default: printf("Invalid choice.\n");
        }
    }
    return 0;
}

// 1. CREATE: Adds a schema-less document
void create_document() {
    // Expand database size
    Document *temp = (Document *)realloc(db, (doc_count + 1) * sizeof(Document));
    if (!temp) { printf("Memory Error.\n"); return; }
    db = temp;

    // Initialize new document
    db[doc_count].fields = NULL;
    db[doc_count].field_count = 0;

    printf("--- New Document (Record %d) ---\n", doc_count);
    printf("Enter fields (e.g., Name=John). Type 'END' as key to finish.\n");

    while (1) {
        char *key = get_dynamic_string("Enter Key (or 'END'): ");
        if (strcmp(key, "END") == 0 || strcmp(key, "end") == 0) {
            free(key); // Free the "END" string
            break;
        }

        char *value = get_dynamic_string("Enter Value: ");

        // Expand field list for this document
        int count = db[doc_count].field_count;
        Field *f_temp = (Field *)realloc(db[doc_count].fields, (count + 1) * sizeof(Field));
        
        if (!f_temp) { printf("Memory Error.\n"); break; }
        db[doc_count].fields = f_temp;

        // Store the dynamic strings
        db[doc_count].fields[count].key = key;
        db[doc_count].fields[count].value = value;
        db[doc_count].field_count++;
    }

    doc_count++;
    printf("Document saved.\n");
}

// 2. READ: Displays data in JSON-like format
void read_documents() {
    if (doc_count == 0) { printf("Database is empty.\n"); return; }

    printf("\n--- Collection Dump ---\n");
    for (int i = 0; i < doc_count; i++) {
        printf("Doc [%d]: { ", i);
        for (int j = 0; j < db[i].field_count; j++) {
            printf("\"%s\": \"%s\"", db[i].fields[j].key, db[i].fields[j].value);
            if (j < db[i].field_count - 1) printf(", ");
        }
        printf(" }\n");
    }
}

// 3. SEARCH: Filter documents by Key-Value pair
void search_documents() {
    if (doc_count == 0) { printf("Database is empty.\n"); return; }

    char *search_key = get_dynamic_string("Search by Key: ");
    char *search_val = get_dynamic_string("Search by Value: ");
    int found = 0;

    printf("\n--- Search Results ---\n");
    for (int i = 0; i < doc_count; i++) {
        for (int j = 0; j < db[i].field_count; j++) {
            if (strcmp(db[i].fields[j].key, search_key) == 0 &&
                strcmp(db[i].fields[j].value, search_val) == 0) {
                
                // Print match
                printf("Found in Doc [%d]: { ", i);
                for (int k = 0; k < db[i].field_count; k++) {
                    printf("\"%s\": \"%s\"", db[i].fields[k].key, db[i].fields[k].value);
                    if (k < db[i].field_count - 1) printf(", ");
                }
                printf(" }\n");
                found = 1;
                break; // Stop checking fields in this doc
            }
        }
    }
    if (!found) printf("No matching documents found.\n");

    free(search_key);
    free(search_val);
}

// 4. UPDATE: Add or Modify fields in a specific document
void update_document() {
    read_documents(); // Show IDs
    int id;
    printf("Enter Doc ID to update: ");
    scanf("%d", &id);
    clear_input_buffer();

    if (id < 0 || id >= doc_count) { printf("Invalid ID.\n"); return; }

    printf("1. Modify existing field\n2. Add new field\nChoice: ");
    int sub_choice;
    scanf("%d", &sub_choice);
    clear_input_buffer();

    if (sub_choice == 1) {
        char *key_to_find = get_dynamic_string("Enter Key to modify: ");
        int modified = 0;
        for(int i=0; i<db[id].field_count; i++) {
            if(strcmp(db[id].fields[i].key, key_to_find) == 0) {
                printf("Current Value: %s\n", db[id].fields[i].value);
                free(db[id].fields[i].value); // Free old value
                db[id].fields[i].value = get_dynamic_string("Enter New Value: ");
                modified = 1;
                printf("Field updated.\n");
                break;
            }
        }
        if(!modified) printf("Key not found in this document.\n");
        free(key_to_find);
    } 
    else if (sub_choice == 2) {
        // Add new field logic (similar to create)
        char *key = get_dynamic_string("Enter New Key: ");
        char *value = get_dynamic_string("Enter New Value: ");
        
        int count = db[id].field_count;
        Field *temp = (Field *)realloc(db[id].fields, (count + 1) * sizeof(Field));
        if(temp) {
            db[id].fields = temp;
            db[id].fields[count].key = key;
            db[id].fields[count].value = value;
            db[id].field_count++;
            printf("Field added.\n");
        }
    }
}

// 5. DELETE: Removes a document by Index
void delete_document() {
    read_documents();
    int id;
    printf("Enter Doc ID to delete: ");
    scanf("%d", &id);
    clear_input_buffer();

    if (id < 0 || id >= doc_count) { printf("Invalid ID.\n"); return; }

    // Free the memory inside the document being deleted
    for (int j = 0; j < db[id].field_count; j++) {
        free(db[id].fields[j].key);
        free(db[id].fields[j].value);
    }
    free(db[id].fields);

    // Shift remaining documents left
    for (int i = id; i < doc_count - 1; i++) {
        db[i] = db[i + 1];
    }

    doc_count--;
    
    // Shrink database memory
    if (doc_count > 0) {
        Document *temp = (Document *)realloc(db, doc_count * sizeof(Document));
        if (temp) db = temp;
    } else {
        free(db);
        db = NULL;
    }
    printf("Document deleted.\n");
}

// Helper: Allocates memory for user string input
char* get_dynamic_string(const char* prompt) {
    char buffer[BUFFER_SIZE];
    printf("%s", prompt);
    if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0'; // Trim newline
        }
        return strdup(buffer); // Allocate exact memory and copy
    }
    return strdup("");
}

// Helper: Free everything
void free_all_memory() {
    for (int i = 0; i < doc_count; i++) {
        for (int j = 0; j < db[i].field_count; j++) {
            free(db[i].fields[j].key);
            free(db[i].fields[j].value);
        }
        free(db[i].fields);
    }
    free(db);
}

void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}
