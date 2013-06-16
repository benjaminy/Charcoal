<?php
function format_line( $line, $max_len )
{
    $trimmed = ltrim( $line, " " );
    $diff = strlen( $line ) - strlen( $trimmed );
    if( $diff > 0 )
    {
        echo '<pre>';
        echo str_repeat( ' ', $diff );
        echo '</pre>';
    }
    echo trim( $line );
    //echo str_repeat( '&nbsp;', $max_len - strlen( $line ) );
}

function format_code( $code )
{
    $code_lines = explode( "\n", $code );
    $max_len = 0;
    foreach( $code_lines as $line )
    {
        $len = strlen( $line );
        $max_len = max( $max_len, $len );
    }
    echo '<div class="highlight mono">
<table cellpadding="0" cellspacing="0"><tbody><tr>
<td align="right">';
    for( $i = 1; $i <= count( $code_lines ); $i++ )
    {
        if( $i % 2 == 0 )
            echo '<span style="background-color:aliceblue">';
        echo "{$i}:";
        if( $i < count( $code_lines ) )
            echo "<br/>\n";
        if( $i % 2 == 0 )
            echo '</span>';
    }
    echo "</td>\n";
    echo '<td>';
    for( $i = 1; $i <= count( $code_lines ); $i++ )
    {
        if( $i % 2 == 0 )
            echo '<span style="background-color:aliceblue">';
        echo "&nbsp;&nbsp;";
        if( $i < count( $code_lines ) )
            echo "<br/>\n";
        if( $i % 2 == 0 )
            echo '</span>';
    }
    echo "</td>\n";
    echo '<td>';
    for( $i = 0; $i < count( $code_lines ); $i++ )
    {
        if( $i % 2 == 1 )
        {
            echo '<span style="display:block;background-color:aliceblue">';
        }
        echo format_line( $code_lines[ $i ], $max_len );
        if( $i < count( $code_lines ) - 1 )
            echo "<br/>\n";
        if( $i % 2 == 1 )
        {
            echo '</span>';
        }
    }
    echo '</td>
</tr></tbody></table>
</div>
';
}
?>
