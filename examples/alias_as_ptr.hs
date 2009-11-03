numero = 123;

reference = &numero;
nuovo     = *reference;

print( "numero    : ".numero." (".typeof(numero).")\n" );
print( "reference : ".reference." (".typeof(reference).")\n" );
print( "nuovo     : ".nuovo." (".typeof(nuovo).")\n\n" );

dll  = dllopen( "./libtest.so" );
if( dll == 0 ){
	print( "error loading library\n" );
	exit();
}

pstruct = dlllink( dll, "print_struct" );
if( pstruct == 0 ){
	print( "error linking symbol\n" );
	exit();
}

test_struct_t = struct( "number" -> numero,
						"str"    -> "Hello World!",
						"dbl"    -> 0.321111 );

ptr = &test_struct_t;

print( "ptr         : ".ptr." (".typeof(ptr).")\n" );

pstruct( ptr );

dllclose(dll);
