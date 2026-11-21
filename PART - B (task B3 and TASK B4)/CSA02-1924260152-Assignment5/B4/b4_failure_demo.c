/* Shows WHY realloc's result goes into a temporary pointer.
 * The address space is limited (ulimit -v) so that realloc eventually fails.
 * Because the result is kept in p, list->data is still valid after the failure:
 * the donors already stored can be saved and the memory freed cleanly.       */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { int id; char group[4]; } Donor;
typedef struct { Donor *data; int size, capacity; } DonorList;

int add_donor(DonorList *list, int id, const char *group)
{
    if (list->size == list->capacity) {
        int new_capacity = (list->capacity == 0) ? 2 : list->capacity * 2;
        Donor *p = realloc(list->data, (size_t)new_capacity * sizeof(Donor));
        if (p == NULL)
            return 0;
        list->data = p;
        list->capacity = new_capacity;
    }
    list->data[list->size].id = id;
    strncpy(list->data[list->size].group, group, 3);
    list->data[list->size].group[3] = '\0';
    list->size++;
    return 1;
}

int main(void)
{
    DonorList list = {NULL, 0, 0};
    int id = 1;
    while (add_donor(&list, id, "O+")) id++;
    printf("add_donor() returned 0 (realloc failed) after %d donors, capacity = %d\n", list.size, list.capacity);

    int intact = 1;                      /* is every stored donor still readable? */
    for (int i = 0; i < list.size; i++)
        if (list.data[i].id != i + 1 || strcmp(list.data[i].group, "O+") != 0) { intact = 0; break; }
    printf("list.data still valid after failure: %s (first id %d, last id %d)\n",
           intact ? "YES - no data lost" : "NO", list.data[0].id, list.data[list.size - 1].id);
    free(list.data);
    printf("memory freed cleanly, program exits normally\n");
    return 0;
}
