#ifndef CHARRAY_H_
#define CHARRAY_H_

typedef struct  
{
    size_t size;
    size_t capacity;
} Header;

#ifdef CHARRAY_IMPLEMENTATION

#define CH_CAPACITY 256

#define CH_ARRAY(t) t*

#define CH_ARRAY_INIT(arr)                                                      \
    do                                                                          \
    {                                                                           \
        Header* header = malloc(sizeof(Header) + sizeof(*arr) * CH_CAPACITY);   \
        header->size = 0;                                                       \
        header->capacity = CH_CAPACITY;                                         \
        arr = (void*)(header + 1);                                              \
    } while (0)                                                                 \

#define CH_ARRAY_LENGTH(arr)   ((Header*)(arr) - 1)->size
#define CH_ARRAY_CAPACITY(arr) ((Header*)(arr) - 1)->capacity
#define CH_ARRAY_EMPTY(arr)    (((Header*)(arr) - 1)->size = 0)
#define CH_ARRAY_IS_EMPTY(arr) (((Header*)(arr) - 1)->size == 0)
#define CH_ARRAY_PUSH(arr, data)                                                        \
    do                                                                                  \
    {                                                                                   \
        if (arr == NULL) CH_ARRAY_INIT(arr);                                            \
        Header* header = (Header*)arr - 1;                                              \
                                                                                        \
        if (header->size >= header->capacity)                                           \
        {                                                                               \
            header->capacity *= 2;                                                      \
            header = realloc(header, sizeof(Header) + sizeof(*arr) * header->capacity); \
            arr = (void*)(header + 1);                                                  \
        }                                                                               \
                                                                                        \
        (arr)[header->size++] = (data);                                                 \
    } while (0)                                                                         

#else

#define CH_ARRAY
#define CH_CAPACITY 
#define CH_ARRAY_INIT
#define CH_ARRAY_LENGTH
#define CH_ARRAY_CAPACITY
#define CH_ARRAY_PUSH

#endif //CHARRAY_IMPLEMENTATIOn

#endif //CHARRAY_H_