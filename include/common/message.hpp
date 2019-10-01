#ifndef _DROPFOLDER_MESSAGE_H_
#define _DROPFOLDER_MESSAGE_H_

#define MESSAGE_MAX_FILENAME_SIZE 256

enum class MessageType {
    DELETE_FILE,
    UPDATE_FILE
};

struct Message {
    MessageType type;
    char filename[MESSAGE_MAX_FILENAME_SIZE];
    unsigned int file_length;
    char bytes[];
};

#endif //  _DROPFOLDER_MESSAGE_H_

