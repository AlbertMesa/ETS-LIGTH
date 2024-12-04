function loguer(){
    let user=document.getElementById("usuario").value;
    let password=document.getElementById("clave").value;

    if(user == "ETSI" && password == "1234"){
        window.location="/backscreen";
    }
    else{
        alert("Datos incorrectos");
    }
}

function loguerout(){
window.location="index.html";
}


let systemOn = false;
let isAutomatic = false;
document.getElementById('toggleSystem').addEventListener('click', () => {
    systemOn = !systemOn;
    alert(`System is now ${systemOn ? 'On' : 'Off'}`);
});

document.getElementById('changeMode').addEventListener('click', () => {
    isAutomatic = !isAutomatic;
    alert(`Operation mode is now ${isAutomatic ? 'Automatic' : 'Manual'}`);
});


