package com.example.terrarium

import android.app.NotificationChannel
import android.app.NotificationManager
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.os.Build
import android.support.v4.app.NotificationCompat

class Alarm : BroadcastReceiver() {
    companion object{
        const val  ID = "CHANNEL_ID"
        const val  chanel_name = "CHANNEL_NAME"
    }
    override fun onReceive(context: Context?, intent: Intent?) {
        val manager = context?.getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
        val channelNotification: NotificationCompat.Builder = NotificationCompat.Builder(context, ID)
            .setContentTitle("!!! ALERT !!!")
            .setContentText("SPRAWDŹ TERRARIUM, WĄŻ W NIEBEZPIECZEŃSTWIE")
            .setSmallIcon(R.drawable.ic_baseline_error_24)
        val chanel = NotificationChannel(ID, chanel_name, NotificationManager.IMPORTANCE_HIGH)

        if (Build.VERSION.SDK_INT>= Build.VERSION_CODES.O){
            manager.createNotificationChannel(chanel)
            manager.notify(1, channelNotification.build())
        }
    }

}