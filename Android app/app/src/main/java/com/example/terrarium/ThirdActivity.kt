package com.example.terrarium

import android.support.v7.app.AppCompatActivity
import android.os.Bundle
import android.widget.Toast
import com.android.volley.Request
import com.android.volley.RequestQueue
import com.android.volley.Response
import com.android.volley.toolbox.JsonObjectRequest
import com.android.volley.toolbox.Volley
import kotlinx.android.synthetic.main.activity_third.*


class ThirdActivity : AppCompatActivity() {
    private var requestQueue: RequestQueue? = null
    var url = "https://esp32-terrarium-control.now.sh/config?temp=32"
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_third)
        requestQueue = Volley.newRequestQueue(this)

        ButSendData.setOnClickListener{
            val ValTemp: String = editTextTemp.text.toString()
            val ValWilg: String = editTextWilg.text.toString()
            if(ValTemp.isNullOrEmpty() && !ValWilg.isNullOrEmpty()){
                url = "https://esp32-terrarium-control.now.sh/config?wilg="+ValWilg
                SendData()
            }
            else if( ValWilg.isNullOrEmpty() && !ValTemp.isNullOrEmpty()){
                url = "https://esp32-terrarium-control.now.sh/config?temp="+ValTemp
                SendData()
            }
            else if (ValTemp.isNullOrEmpty() && ValWilg.isNullOrEmpty()) {
                val myToast = Toast.makeText(applicationContext,"Wprowadź dane", Toast.LENGTH_SHORT)
                myToast.show()
            }
            else{
                url = "https://esp32-terrarium-control.now.sh/config?temp="+ValTemp+"&wilg="+ValWilg
                SendData()
            }

        }

    }
    private fun SendData(){

        val request = JsonObjectRequest(Request.Method.GET, url, null, Response.Listener  {
                response ->
            val myToast = Toast.makeText(applicationContext,"Wysłano", Toast.LENGTH_SHORT)
            myToast.show()
        }, Response.ErrorListener { error -> error.printStackTrace() })
        requestQueue?.add(request)
    }
}
