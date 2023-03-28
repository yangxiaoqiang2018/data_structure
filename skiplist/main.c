# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <sys/time.h>

# include "ut_skiplist.h"

double current_timestamp(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + tv.tv_usec / 1000000.0;
}

void *node_dup(void *obj)
{
    return strdup(obj);
}

void node_free(void *obj)
{
    free(obj);
}

int node_compare(const void *obj, const void *key)
{
    return strcmp(obj, key);
}

void skiplist_view(skiplist_t *list)
{
    printf("###########################\n");
    printf("list len: %ld\n", skiplist_len(list));
    printf("level: %d\n", list->level);

    for (int i = list->level - 1; i >= 0; --i) {
        printf("level %d: ", i);
        skiplist_node *node = list->header->level[i].forward;
        printf("H %d -> ", list->header->level[i].span);
        while (node) {
            printf("%s %d -> ", (char *)node->value, node->level[i].span);
            node = node->level[i].forward;
        }
        
        printf("\n");
    }
}

int main(int argc, char *argv[])
{
    skiplist_type type;
    type.dup = node_dup;
    type.free = node_free;
    type.compare = node_compare;
    skiplist_t *list = skiplist_create(&type);

    int rank = 0;
    for (int i = 0; i < 30; ++i) {
        int num = random() % 26;
        char value[100] = {0};
        sprintf(value, "%c", 'a' + num);
        if (skiplist_insert(list, value, &rank)) {
            printf("insert: %s, rank: %d\n", value, rank);
        }
    }
    
    /*
    for (int i = 0; i < 26; ++i) {
        char value[100] = {0};
        sprintf(value, "%c", 'a' + i);
        if (skiplist_insert(list, value, &rank)) {
            printf("insert: %s, rank: %d\n", value, rank);
        }
    }
    */

    skiplist_view(list);

    skiplist_node *tail = skiplist_tail(list);
    printf("tail: %s\n", (char *)tail->value);

    char *value = "a";
    rank = 0;
    skiplist_node *node = skiplist_find(list, value, &rank);
    if (node) {
        printf("find %s, rank: %d\n", value, rank);
        //skiplist_delete(list, node);
    }
    
    skiplist_view(list);

    tail = skiplist_tail(list);
    printf("tail: %s\n", (char *)tail->value);

    char *value_tmp = NULL;
    double start = current_timestamp();
    skiplist_iter *iter = skiplist_get_iterator(list);
    while ((node = skiplist_next(iter)) != NULL) {
        value_tmp = node->value;
        rank = 0;
        skiplist_node *node = skiplist_find(list, value_tmp, &rank);
        printf("find %s rank: %d\n", value_tmp, rank);
        //skiplist_delete(list, node);
    }
    skiplist_release_iterator(iter);
    double end = current_timestamp();

    printf("iter cost: %lf\n", end - start);

    printf("list len: %ld\n", skiplist_len(list));
    skiplist_release(list);
    
    printf("size: %zu\n", sizeof(struct skiplist_level));

    return 0;
}

