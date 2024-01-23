package com.example.proyectose

import android.content.Intent
import android.os.Bundle
import android.view.View
import android.widget.Button
import android.widget.EditText
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import com.google.firebase.database.DataSnapshot
import com.google.firebase.database.DatabaseError
import com.google.firebase.database.DatabaseReference
import com.google.firebase.database.FirebaseDatabase
import com.google.firebase.database.ValueEventListener

class MainActivity : AppCompatActivity() {
    private var editTextEmail: EditText? = null
    private var editTextPassword: EditText? = null
    private lateinit var buttonLogin: Button
    private var databaseReference: DatabaseReference? = null
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        editTextEmail = findViewById<EditText>(R.id.emailEditText)
        editTextPassword = findViewById<EditText>(R.id.passwordEditText)
        buttonLogin = findViewById<Button>(R.id.loginButton)

        // Inicializa la referencia de Firebase
        databaseReference = FirebaseDatabase.getInstance().getReference("usuarios")
        buttonLogin.setOnClickListener(View.OnClickListener { login() })
    }

    private fun login() {
        val email = editTextEmail!!.text.toString().trim { it <= ' ' }
        val password = editTextPassword!!.text.toString().trim { it <= ' ' }
        if (email.isEmpty()) {
            editTextEmail!!.error = "Email es requerido"
            editTextEmail!!.requestFocus()
            return
        }
        if (password.isEmpty()) {
            editTextPassword!!.error = "Contraseña es requerida"
            editTextPassword!!.requestFocus()
            return
        }

        databaseReference?.orderByChild("email")?.equalTo(email)
            ?.addListenerForSingleValueEvent(object : ValueEventListener {
                override fun onDataChange(dataSnapshot: DataSnapshot) {
                    if (dataSnapshot.exists()) {
                        for (userSnapshot in dataSnapshot.children) {
                            val fetchedPassword: String? = userSnapshot.child("contrasena").getValue(
                                String::class.java
                            )
                            if (fetchedPassword != null && fetchedPassword == password) {
                                Toast.makeText(
                                    this@MainActivity,
                                    "Inicio de sesión exitoso",
                                    Toast.LENGTH_SHORT
                                ).show()
                                accessToProfile()
                            } else {
                                Toast.makeText(
                                    this@MainActivity,
                                    "Contraseña incorrecta",
                                    Toast.LENGTH_SHORT
                                ).show()
                            }
                        }
                    } else {
                        Toast.makeText(
                            this@MainActivity,
                            "Usuario no encontrado",
                            Toast.LENGTH_SHORT
                        ).show()
                    }
                }

                override fun onCancelled(databaseError: DatabaseError) {
                    Toast.makeText(
                        this@MainActivity,
                        "Error: " + databaseError.message,
                        Toast.LENGTH_SHORT
                    ).show()
                }
            })
    }

    private fun accessToProfile(){
        val i = Intent(this, Perfil::class.java)
        i.putExtra("nombreusuario", editTextEmail?.text.toString())
        startActivity(i);
    }
}