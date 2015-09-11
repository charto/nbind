var nbind = require( 'nbind' ).init( __dirname );
var Person = nbind.module.Person;

var zaphod = new Person( 'Zaphod' );
var trillian = new Person( 'Trillian' )

zaphod.friend = trillian;

function PointJS( x, y ) {
    this.x = x;
    this.y = y;
}

PointJS.prototype.fromJS = function( output ) {
    output( this.x, this.y );
}

nbind.bind( 'MyPoint', PointJS );

Person.beAmazing( toConsole );

/*
zaphod.eatAt(
    new PointJS( 60.17, 24.94 ),
    [ 'Pasta', 'Cake' ]
);
*/

toConsole( 'Same ' + zaphod.friend + '? ' + ( zaphod.friend == trillian ) );

function toConsole( message ) {
    console.log( message );
}
