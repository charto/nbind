var Point=require('bindings')('example').Point;

var a=Point();
var b=new Point(1,2);
var c=Point(10,20);

c.add(b);

a.print();
b.print();
c.print();
