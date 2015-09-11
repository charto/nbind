#include "nbind/binding.h"
#include "example.h"

NBIND_CLASS(Person) {
    construct<>();
    construct<const char *>();

    getter(get_name);
    getset(getFriend, setFriend);

    method(toString);
    method(beAmazing);
    method(eatAt);
}
