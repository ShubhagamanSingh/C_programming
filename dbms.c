/*
 * Dynamic Database Management System in C
 * ---------------------------------------
 * Features:
 * - Fully user-dependent menu.
 * - Uses malloc/realloc to manage memory dynamically (no fixed size arrays).
 * - Implements CRUD (Create, Read, Update, Delete) operations.
 * - Handles input buffer clearing for stable string input.
   * ---------------------------------------
  * Author:
  * - Shubhagaman Singh
  * - https://shubhagaman-74c28.web.app
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define the structure for a record (e.g., Student)
typedef struct {
    int id;
    char name[50];
    char department[30];
    float gpa;
} Student;

// Global pointers to maintain the dynamic database
Student *db = NULL;
int record_count = 0;

// Function Prototypes
void add_record();
void display_all();
void search_record();
void update_record();
void delete_record();
void free_memory();
int find_index_by_id(int id);
void clear_input_buffer();

int main() {
    int choice;

    printf("=== Dynamic Database Management System ===\n");
    printf("Storage is allocated dynamically based on your usage.\n");

    while (1) {
        printf("\n--- Main Menu ---\n");
        printf("1. Add Record\n");
        printf("2. Display All Records\n");
        printf("3. Search Record by ID\n");
        printf("4. Update Record\n");
        printf("5. Delete Record\n");
        printf("6. Exit\n");
        printf("Enter your choice: ");
        
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            clear_input_buffer();
            continue;
        }
        clear_input_buffer(); // Consume newline left by scanf

        switch (choice) {
            case 1: add_record(); break;
            case 2: display_all(); break;
            case 3: search_record(); break;
            case 4: update_record(); break;
            case 5: delete_record(); break;
            case 6:
                free_memory();
                printf("Exiting... Memory freed.\n");
                exit(0);
            default:
                printf("Invalid choice! Try again.\n");
        }
    }
    return 0;
}

// 1. CREATE: Add a new record dynamically
void add_record() {
    // Check if ID already exists
    int new_id;
    printf("Enter Student ID: ");
    scanf("%d", &new_id);
    clear_input_buffer();

    if (find_index_by_id(new_id) != -1) {
        printf("Error: Record with ID %d already exists!\n", new_id);
        return;
    }

    // Reallocate memory to fit one more record
    // If db is NULL, realloc acts like malloc
    Student *temp = (Student *)realloc(db, (record_count + 1) * sizeof(Student));
    
    if (temp == NULL) {
        printf("Critical Error: Memory allocation failed! System out of memory.\n");
        return;
    }
    db = temp; // Update global pointer

    // Store the ID we already captured
    db[record_count].id = new_id;

    // Get rest of the data
    printf("Enter Name: ");
    scanf("%49[^\n]", db[record_count].name); // Read until newline
    clear_input_buffer();

    printf("Enter Department: ");
    scanf("%29[^\n]", db[record_count].department);
    clear_input_buffer();

    printf("Enter GPA: ");
    scanf("%f", &db[record_count].gpa);
    clear_input_buffer();

    record_count++;
    printf("Record added successfully! Total records: %d\n", record_count);
}

// 2. READ: Display all records
void display_all() {
    if (record_count == 0) {
        printf("Database is empty.\n");
        return;
    }

    printf("\n%-10s %-25s %-20s %-10s\n", "ID", "Name", "Department", "GPA");
    printf("------------------------------------------------------------------\n");
    for (int i = 0; i < record_count; i++) {
        printf("%-10d %-25s %-20s %-10.2f\n", 
               db[i].id, db[i].name, db[i].department, db[i].gpa);
    }
    printf("------------------------------------------------------------------\n");
}

// 3. SEARCH: Find specific record
void search_record() {
    int id;
    printf("Enter ID to search: ");
    scanf("%d", &id);
    clear_input_buffer();

    int index = find_index_by_id(id);
    if (index == -1) {
        printf("Record not found.\n");
    } else {
        printf("\nRecord Found:\n");
        printf("ID: %d\n", db[index].id);
        printf("Name: %s\n", db[index].name);
        printf("Dept: %s\n", db[index].department);
        printf("GPA: %.2f\n", db[index].gpa);
    }
}

// 4. UPDATE: Modify existing record
void update_record() {
    int id;
    printf("Enter ID to update: ");
    scanf("%d", &id);
    clear_input_buffer();

    int index = find_index_by_id(id);
    if (index == -1) {
        printf("Record not found. Cannot update.\n");
        return;
    }

    printf("Updating Record for %s (ID: %d)\n", db[index].name, db[index].id);
    
    printf("Enter New Name: ");
    scanf("%49[^\n]", db[index].name);
    clear_input_buffer();

    printf("Enter New Department: ");
    scanf("%29[^\n]", db[index].department);
    clear_input_buffer();

    printf("Enter New GPA: ");
    scanf("%f", &db[index].gpa);
    clear_input_buffer();

    printf("Record updated successfully.\n");
}

// 5. DELETE: Remove record and shrink memory
void delete_record() {
    int id;
    printf("Enter ID to delete: ");
    scanf("%d", &id);
    clear_input_buffer();

    int index = find_index_by_id(id);
    if (index == -1) {
        printf("Record not found. Cannot delete.\n");
        return;
    }

    // Logic: Shift all records after 'index' one position to the left
    // to overwrite the deleted record.
    for (int i = index; i < record_count - 1; i++) {
        db[i] = db[i + 1];
    }

    record_count--;

    if (record_count > 0) {
        // Shrink memory block
        Student *temp = (Student *)realloc(db, record_count * sizeof(Student));
        if (temp != NULL) {
            db = temp;
        }
        // If temp is NULL during shrink, we usually keep the old block (it's slightly larger but safe)
        // but ideally, realloc shrinking rarely fails.
    } else {
        // If no records left, free completely
        free(db);
        db = NULL;
    }

    printf("Record deleted successfully. Total records: %d\n", record_count);
}

// Helper: Find array index by ID
int find_index_by_id(int id) {
    for (int i = 0; i < record_count; i++) {
        if (db[i].id == id) {
            return i;
        }
    }
    return -1;
}

// Helper: Clean up all memory on exit
void free_memory() {
    if (db != NULL) {
        free(db);
        db = NULL;
    }
}

// Helper: Clear stdin buffer to prevent scanf skipping inputs
void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}
