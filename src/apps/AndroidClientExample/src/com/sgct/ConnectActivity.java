package com.sgct;

import android.widget.CheckBox;
import android.widget.Toast;
import com.sgct.HelperClasses.Connector;
import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;


import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.UnknownHostException;

/**
 * Helps with connection settings. Reads Ip and port number and passes it along to the main activity
 * User: alefr38
 * Date: 2012-02-22
 */
public class ConnectActivity extends Activity {

    //----------------------------------------------------------------------------//
    // Class variable declaration
    //----------------------------------------------------------------------------//

    /** Helper class to connect to host */
    Connector mConnector = Connector.getInstance();

    //----------------------------------------------------------------------------//
    // Event handlers
    //----------------------------------------------------------------------------//

    /**
     * Saves variables in activity intent, set result to OK
     * and finish activity
     */
    private OnClickListener mConnect = new OnClickListener() {

        @Override
        public void onClick(View v) {

            // Printing error messages handling in connectToHost
            if( !connectToHost() ) {
                return;
            }
            
            Bundle b = new Bundle();
            b.putBoolean( "autoconnect",
                    ((CheckBox)findViewById(R.id.autoconnectCB)).isChecked() );

            Intent i = new Intent();
            i.putExtras( b );

            setResult( RESULT_OK, i );
            finish();
        }
    };

    /**
     * Cancels connection and returns Cancel code as result
     * and finish activity
     */
    private OnClickListener mCancel = new OnClickListener() {

        @Override
        public void onClick(View v) {

            setResult( RESULT_CANCELED );
            finish();
        }
    };

    //----------------------------------------------------------------------------//
    // Class member functions
    //----------------------------------------------------------------------------//
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.connect);

        // Should always have a bundle passed to the activity
        readConnectionValues(getIntent().getExtras());

        Button btn = (Button)findViewById(R.id.connectBTN);
        btn.setOnClickListener(mConnect);

        btn = (Button)findViewById(R.id.cancelBTN);
        btn.setOnClickListener(mCancel);

    }

    /**
     * Builds host address from input fields. All empty inputs will be replaced with 0
     * @return  Host IP4 address
     */
    private String getHostAddress()
    {
        EditText et = (EditText)findViewById(R.id.ip1ET);
        String ip1 = (et.getText().length() > 0) ? et.getText().toString() : "0";

        et = (EditText)findViewById(R.id.ip2ET);
        String ip2 = (et.getText().length() > 0) ? et.getText().toString() : "0";

        et = (EditText)findViewById(R.id.ip3ET);
        String ip3 = (et.getText().length() > 0) ? et.getText().toString() : "0";

        et = (EditText)findViewById(R.id.ip4ET);
        String ip4 = (et.getText().length() > 0) ? et.getText().toString() : "0";

        return ip1+"."+ip2+"."+ip3+"."+ip4;
    }

    /**
     * Will get the host port by reading from the input field. Set to 0 if empty
     * @return Host port, 0 if empty
     */
    private int getHostPort()
    {
        EditText et = (EditText)findViewById(R.id.portET);
        return (et.getText().length() > 0) ? Integer.parseInt(et.getText().toString() ) : 0;
    }

    /**
     * Sets the connection widget values from a bundle and the previous connection
     * @param	b	Bundle to read values from
     */
    private void readConnectionValues( Bundle b )
    {
        CheckBox cb = (CheckBox)findViewById(R.id.autoconnectCB);
        cb.setChecked( b.getBoolean( "autoconnect", false ) );

        String[] ip = mConnector.getAddress().split(".");

        // Don't fill in connection values if none have been previously specified
        if( ip.length < 4 || (ip[0].equals("0") && ip[1].equals("0") && ip[2].equals("0") && ip[3].equals("0")) ) {
            return;
        }

        EditText et = (EditText)findViewById(R.id.ip1ET);
        et.setText(ip[0]);

        et = (EditText)findViewById(R.id.ip2ET);
        et.setText(ip[1]);

        et = (EditText)findViewById(R.id.ip3ET);
        et.setText(ip[2]);

        et = (EditText)findViewById(R.id.ip4ET);
        et.setText(ip[3]);

        et = (EditText)findViewById(R.id.portET);
        et.setText( Integer.toString( mConnector.getPort() ) );
    }

    /**
     * Attempts to connect to the host with the values in the input fields
     * @return If connection went ok
     */
    private boolean connectToHost(){
        String address = getHostAddress();
        int port = getHostPort();
        
        Toast.makeText( getApplicationContext(),"Connecting to host: " + address + ":" + port,
                Toast.LENGTH_SHORT ).show();

        if( !mConnector.connect(address, port, 5000) ){
            Toast.makeText( getApplicationContext(),"Connection failed: " + mConnector.getErrorMessage(),
                    Toast.LENGTH_SHORT ).show();
            return false;
        }

        Toast.makeText( getApplicationContext(),"Connection established", Toast.LENGTH_SHORT ).show();
        return true;
    }
}