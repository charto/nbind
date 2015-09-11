#include "nbind/api.h"

struct MyPoint {
    MyPoint(double x, double y) :
        x(x), y(y) {}

    void toJS(nbind::cbOutput output) {
        output(x, y);
    }

    double x, y;
};

class Person {

public:

    Person() {}
    Person(const char *name) :
        name(strdup(name)) {}

    const char *toString() { return(name); }
    const char *get_name() { return(name); }

    Person *getFriend() { return(bestFriend); }
    void setFriend(Person *someone) { bestFriend = someone; }

    static void beAmazing(nbind::cbFunction &callback) {
        callback("I'm amazing!");
    }

    void eatAt(MyPoint pt, std::vector<std::string> foods) {}

private:

    const char *name = nullptr;
    Person *bestFriend;
};
