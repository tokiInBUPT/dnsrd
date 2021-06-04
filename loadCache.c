#include "loadCache.h"

void loadCache(char *fname, DNSRD_RUNTIME *runtime) {
    yyjson_doc *doc = yyjson_read_file(fname, 0, NULL, NULL);
    yyjson_val *root = yyjson_doc_get_root(doc);
    size_t idx, max;
    yyjson_val *objectItem;
    yyjson_arr_foreach(root, idx, max, objectItem) {
        Key key;
        MyData value;
        yyjson_val *keyJson = yyjson_obj_get(objectItem, "key");
        yyjson_val *valueJson = yyjson_obj_get(objectItem, "value");
        yyjson_val *val = yyjson_obj_get(keyJson, "qtype");
        key.qtype = (DNSQType)yyjson_get_uint(val);
        val = yyjson_obj_get(keyJson, "name");
        strcpy_s(key.name, 256, yyjson_get_str(val));
        val = yyjson_obj_get(valueJson, "time");
        value.time = yyjson_get_uint(val);
        val = yyjson_obj_get(valueJson, "answerCount");
        value.answerCount = (uint32_t)yyjson_get_uint(val);
        yyjson_val *answers = yyjson_obj_get(valueJson, "answers");
        value.answers = (DNSRecord *)malloc(sizeof(DNSRecord) * (value.answerCount));
        for (uint64_t i = 0; i < value.answerCount; i++) {
            yyjson_val *answerItemJson = yyjson_arr_get(answers, i);
            val = yyjson_obj_get(answerItemJson, "name");
            strcpy_s(value.answers[i].name, 256, yyjson_get_str(val));
            val = yyjson_obj_get(answerItemJson, "rclass");
            value.answers[i].rclass = yyjson_get_uint(val);
            val = yyjson_obj_get(answerItemJson, "rdataLength");
            value.answers[i].rdataLength = (uint16_t)yyjson_get_uint(val);
            value.answers[i].rdata = (char *)malloc(sizeof(char) * value.answers[i].rdataLength);
            val = yyjson_obj_get(answerItemJson, "ttl");
            value.answers[i].ttl = (uint32_t)yyjson_get_uint(val);
            val = yyjson_obj_get(answerItemJson, "type");
            value.answers[i].type = yyjson_get_uint(val);
            val = yyjson_obj_get(answerItemJson, "rdata");
            char rdata[4096] = "";
            strcpy_s(rdata, 4096, yyjson_get_str(val));
            for (int j = 0; j < value.answers[i].rdataLength; j++) {
                char *tmp = (char *)malloc(sizeof(char) * 3);
                sscanf(rdata + 3 * j, "%2x", tmp);
                value.answers[i].rdata[j] = *tmp;
            }
            val = yyjson_obj_get(answerItemJson, "rdataName");
            strcpy_s(value.answers[i].rdataName, 256, yyjson_get_str(val));
        }
        lRUCachePut(runtime->lruCache, key, value);
    }
    printf("Cache file loaded, cache size id %d\n", runtime->lruCache->size);
}

void writeCache(char *fname, DNSRD_RUNTIME *runtime) {
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root = yyjson_mut_arr(doc);
    yyjson_mut_doc_set_root(doc, root);
    struct node *head = runtime->lruCache->head;
    head = head->next;
    while (head != runtime->lruCache->tail) {
        yyjson_mut_val *objItem = yyjson_mut_obj(doc);
        yyjson_mut_val *key = yyjson_mut_obj(doc);
        yyjson_mut_obj_add_uint(doc, key, "qtype", head->key.qtype);
        yyjson_mut_obj_add_strcpy(doc, key, "name", head->key.name);
        yyjson_mut_obj_add_val(doc, objItem, "key", key);
        yyjson_mut_val *answers = yyjson_mut_arr(doc);
        for (int i = 0; i < head->value.answerCount; i++) {
            yyjson_mut_val *answerItem = yyjson_mut_obj(doc);
            yyjson_mut_obj_add_uint(doc, answerItem, "ttl", head->value.answers[i].ttl);
            yyjson_mut_obj_add_strcpy(doc, answerItem, "name", head->value.answers[i].name);
            yyjson_mut_obj_add_strcpy(doc, answerItem, "rdataName", head->value.answers[i].rdataName);
            yyjson_mut_obj_add_uint(doc, answerItem, "rclass", head->value.answers[i].rclass);
            yyjson_mut_obj_add_uint(doc, answerItem, "rdataLength", head->value.answers[i].rdataLength);
            yyjson_mut_obj_add_uint(doc, answerItem, "type", head->value.answers[i].type);
            char rdata[4096] = "";
            int j;
            for (j = 0; j < head->value.answers[i].rdataLength; j++) {
                char hex[3];
                sprintf(hex, "%02x ", (unsigned char)head->value.answers[i].rdata[j]);
                strcat(rdata, hex);
            }
            rdata[3 * j] = '\0';
            yyjson_mut_obj_add_strcpy(doc, answerItem, "rdata", rdata);
            yyjson_mut_arr_add_val(answers, answerItem);
        }
        yyjson_mut_val *value = yyjson_mut_obj(doc);
        yyjson_mut_obj_add_uint(doc, value, "time", head->value.time);
        yyjson_mut_obj_add_uint(doc, value, "answerCount", head->value.answerCount);
        yyjson_mut_obj_add_val(doc, value, "answers", answers);
        yyjson_mut_obj_add_val(doc, objItem, "value", value);
        yyjson_mut_arr_add_val(root, objItem);
        head = head->next;
    }
    yyjson_mut_write_file(fname, doc, YYJSON_WRITE_PRETTY, NULL, NULL);
    yyjson_mut_doc_free(doc);
}