#pragma once

void message_queue_send_next();
void message_queue_send(Tuplet tuplets[], int tuplet_count);
void message_queue_send_tuplet(Tuplet tuplet);
void message_queue_sent_handler(DictionaryIterator *iter, void *context);
void message_queue_failed_handler(DictionaryIterator *iter, AppMessageResult reason, void *context);