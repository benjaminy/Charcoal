function testCallbacks(numCallbacks)
{
   var cascade = {};
   for(var i = 0; i < numCallbacks; i++)
   {
      cascade[i] = 
         function(x, y)
            {
                var cascadeTime = 1000 // 1 second
                var interval = cascadeTime / numCallbacks;
                var contractionTime = interval * x;
               setTimeout(cascade[x + 1], contractionTime, x + 1);  
               console.log("Callback: " + x);       
            };
   }
   
   var terminatingAlert = recursiveFun; 
   cascade[numCallbacks] = recursiveFun;
   cascade[0](0);
}

function recursiveFun(iterations)
{
    var absolute = Math.abs(iterations);
    console.log("Recurse1!!! " + iterations);
    if(absolute > 0) {return recursiveFun(absolute - 1);}
    else return alert("The End");
}

