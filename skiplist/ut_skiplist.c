
# include <stdlib.h>
# include <string.h>

# include "ut_skiplist.h"

# define SKIPLIST_MAX_LEVEL 16
# define SKIPLIST_P         0.25

static skiplist_node *skiplist_create_node(skiplist_t *list, int level, void *value)
{
    size_t size = sizeof(skiplist_node) + level * sizeof(struct skiplist_level);
    skiplist_node *node = malloc(size);
    if (node == NULL) {
        return NULL;
    }
    memset(node, 0, size);
    if (value && list->type.dup) {
        node->value = list->type.dup(value);
    } else {
        node->value = value;
    }
    return node;
}

skiplist_t *skiplist_create(skiplist_type *type)
{
    if (type == NULL || type->compare == NULL) {
        return NULL;
    }
    skiplist_t *list = malloc(sizeof(skiplist_t));
    if (list == NULL) {
        return NULL;
    }
    memset(list, 0, sizeof(skiplist_t));
    list->level = 1;
    memcpy(&list->type, type, sizeof(skiplist_type));
    list->header = skiplist_create_node(list, SKIPLIST_MAX_LEVEL, NULL);
    if (list->header == NULL) {
        free(list);
        return NULL;
    }

    for (int i = 0; i < SKIPLIST_MAX_LEVEL; i++) {
        list->header->level[i].forward = NULL;
        list->header->level[i].span = 0;
    }
    list->tail = NULL;
    return list;
}

static int skiplist_random_level(void)
{
    int level = 1;
    while ((random() & 0xFFFF) < (SKIPLIST_P * 0xFFFF)) {
        level += 1;
    }
    return level < SKIPLIST_MAX_LEVEL ? level : SKIPLIST_MAX_LEVEL;
}

skiplist_t *skiplist_insert(skiplist_t *list, void *value, int *node_rank)
{
    skiplist_node *update[SKIPLIST_MAX_LEVEL];
    skiplist_node *node = list->header;
    int rank[SKIPLIST_MAX_LEVEL];

    for (int i = list->level - 1; i >= 0; i--) {
        rank[i] = i == (list->level-1) ? 0 : rank[i+1];
        while (node->level[i].forward && list->type.compare(node->level[i].forward->value, value) <= 0) {
            rank[i] += node->level[i].span;
            node = node->level[i].forward;
        }
        update[i] = node;
    }
    if (node->value && list->type.compare(node->value, value) == 0) {
        return NULL;
    }

    int level = skiplist_random_level();
    if (level > list->level) {
        for (int i = list->level; i < level; ++i) {
            rank[i] = 0;
            update[i] = list->header;
            update[i]->level[i].span = list->len;
        }
        list->level = level;
    }

    node = skiplist_create_node(list, level, value);
    if (node == NULL) {
        return NULL;
    }
    for (int i = 0; i < level; ++i) {
        node->level[i].forward = update[i]->level[i].forward;
        update[i]->level[i].forward = node;

        node->level[i].span = update[i]->level[i].span - (rank[0] - rank[i]);
        update[i]->level[i].span = rank[0] - rank[i] + 1;
    }

    for (int i = level; i < list->level; i++) {
        update[i]->level[i].span++;
    }

    if (node->level[0].forward == NULL) {
        list->tail = node;
    }
    list->len += 1;
    if (node_rank) *node_rank = rank[0] + 1;
    return list;
}

skiplist_node *skiplist_find(skiplist_t *list, void *value, int *node_rank)
{
    skiplist_node *node = list->header;
    for (int i = list->level - 1; i >= 0; i--) {
        while (node->level[i].forward && list->type.compare(node->level[i].forward->value, value) <= 0) {
            if (node_rank) *node_rank += node->level[i].span;
            node = node->level[i].forward;
        }
    }
    if (node->value && list->type.compare(node->value, value) == 0) {
        return node;
    }
    return NULL;
}

void skiplist_delete(skiplist_t *list, skiplist_node *x)
{
    skiplist_node *update[SKIPLIST_MAX_LEVEL];
    skiplist_node *node = list->header;

    for (int i = list->level - 1; i >= 0; i--) {
        while (node->level[i].forward && list->type.compare(node->level[i].forward->value, x->value) < 0) {
            node = node->level[i].forward;
        }
        update[i] = node;
    }

    for (int i = 0; i < list->level; ++i) {
        if (update[i]->level[i].forward == x) {
            update[i]->level[i].span += x->level[i].span - 1;
            update[i]->level[i].forward = x->level[i].forward;
        } else {
            update[i]->level[i].span -= 1;
        }
    }
    while (list->level > 1 && list->header->level[list->level - 1].forward == NULL) {
        list->level -= 1;
    }

    if (x->level[0].forward == NULL) {
        list->tail = update[0];
    }

    if (list->type.free) {
        list->type.free(x->value);
    }
    free(x);
    list->len -= 1;
}

void skiplist_release(skiplist_t *list)
{
    unsigned long len = list->len;
    skiplist_node *curr = list->header->level[0].forward;
    skiplist_node *next;
    while (len--) {
        next = curr->level[0].forward;
        if (list->type.free) {
            list->type.free(curr->value);
        }
        free(curr);
        curr = next;
    }
    free(list->header);
    free(list);
}

skiplist_iter *skiplist_get_iterator(skiplist_t *list)
{
    skiplist_iter *iter = malloc(sizeof(skiplist_iter));
    if (iter == NULL) {
        return NULL;
    }
    iter->next = list->header->level[0].forward;
    return iter;
}

skiplist_node *skiplist_next(skiplist_iter *iter)
{
    skiplist_node *curr = iter->next;
    if (curr) {
        iter->next = curr->level[0].forward;
    }
    return curr;
}

void skiplist_release_iterator(skiplist_iter *iter)
{
    free(iter);
}

skiplist_iter* skiplist_reset_iterator(skiplist_t *list, skiplist_iter *iter)
{
    iter->next = list->header->level[0].forward;
    return iter;
}

