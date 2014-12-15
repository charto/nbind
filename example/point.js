var Point=require('./build/Release/example.node').Point;

var a=Point();
var b=new Point(1,2);
var c=Point(10,20);

c.add(b);

a.print();
b.print();
c.print();
