package com.sgct;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;
import android.content.SharedPreferences;
import com.sgct.HelperClasses.Connector;

public class ClientActivity extends Activity
{
    //----------------------------------------------------------------------------//
    // Class variable declaration
    //----------------------------------------------------------------------------//

    /** Intent id's */
    static final int CONNECT_INTENT = 1;

    /** Helper class to connect to client */
    Connector mConnector = Connector.getInstance();

    /*! If the client should try auto connecting on application startup */
    boolean mAutoConnect = false;

    //----------------------------------------------------------------------------//
    // Event handlers
    //----------------------------------------------------------------------------//

    /**
     * Will open the activity for setting server connection parameters
     */
    private OnClickListener mOpenConnectActivity = new OnClickListener() {

        @Override
        public void onClick(View v) {

            // Create bundle with saved parameters for passing to activity
            Bundle b = new Bundle();

            writeConnectionSettings( b );

            Intent i = new Intent( getApplicationContext(), ConnectActivity.class );
            i.putExtras( b );

            startActivityForResult( i, CONNECT_INTENT );
        }
    };

    /**
     * Will disconnect the current connector if it is connected
     */
    private OnClickListener mDisconnect = new OnClickListener() {

        @Override
        public void onClick(View view) {
            mConnector.disconnect();
            setCorrectLayout();
        }
    };

    /**
     * Send data to the connected host
     */
    private OnClickListener mSendData = new OnClickListener() {
        
        @Override
        public void onClick(View view) {
            EditText et = (EditText)findViewById(R.id.serverMsgET);
            if( !mConnector.send( et.getText().toString() ) )
            {
                Toast.makeText( getApplicationContext(),
                        "Could not send data:" + mConnector.getErrorMessage(),
                        Toast.LENGTH_SHORT ).show();
            }
        }
    };

    //----------------------------------------------------------------------------//
    // Class member functions
    //----------------------------------------------------------------------------//

    /**
     * Called when the activity is first created.
     */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setCorrectLayout();

        // Will initiate the activity by reading from persistent data
        initiateActivity(getPreferences(MODE_PRIVATE));

        setCorrectLayout();
    }

    /**
     * Initiate the activity by reading from persistent data
     * @param sp	SharedPreferences to read from to
     */
    private void initiateActivity( SharedPreferences sp )
    {
        mAutoConnect = sp.getBoolean("autoconnect", false);
        String address = sp.getString("hostip", "0.0.0.0");
        int port = sp.getInt("hostport", 0);

        if( mAutoConnect )
        {
            if( !mConnector.connect(address, port, 5000) )
            {
                Toast.makeText(
                        getApplicationContext(),
                        "Auto connection to host (" + mConnector.getAddress() + ":" + mConnector.getPort() + ") failed: \r\n" +
                            mConnector.getErrorMessage(),
                        Toast.LENGTH_LONG ).show();
            }
            else
            {
                Toast.makeText(
                        getApplicationContext(),
                        "Auto connection to " + mConnector.getAddress() + ":" + mConnector.getPort() + " established",
                        Toast.LENGTH_LONG ).show();
            }
        }
    }

    /**
     * Parse results from other activities
     */
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        // TODO Auto-generated method stub
        super.onActivityResult(requestCode, resultCode, data);

        // Parse results from connection activity
        if( requestCode == CONNECT_INTENT && resultCode == RESULT_OK )
        {
            readConnectionSettings( data.getExtras() );

            // Save settings right away if application crashes
            writeConnectionSettings( getPreferences(MODE_PRIVATE) );

            setCorrectLayout();
        }
    }

    /**
     * Disconnect from server if connected when being destroyed
     */
    @Override
    protected void onDestroy() {
        mConnector.disconnect();
        super.onDestroy();
    }

     /**
     * Sets different layouts based on if client is currently connected or not
     */
    private void setCorrectLayout()
    {
        if( mConnector.isConnected() )
        {
            setContentView(R.layout.main);
            
            Button btn = (Button)findViewById( R.id.sendBTN );
            btn.setOnClickListener( mSendData );
            
            btn = (Button)findViewById( R.id.disconnectBTN );
            btn.setOnClickListener( mDisconnect );
        }
        else
        {
            setContentView(R.layout.unconnected);

            Button btn = (Button)findViewById( R.id.connectBTN );
            btn.setOnClickListener( mOpenConnectActivity );
        }
    }

    /**
     * Helper function for writing connection settings to persistent data
     * @param sp	SharedPreferences to write to
     */
    private void writeConnectionSettings( SharedPreferences sp )
    {
        SharedPreferences.Editor editor = sp.edit();
        editor.putString( "hostip", mConnector.getAddress() );
        editor.putInt("hostport", mConnector.getPort());
        editor.putBoolean( "autoconnect", mAutoConnect );
        editor.commit();
    }

    /**
     * Helper function for writing connection settings to bundle data
     * @param b	Bundle data to write to
     */
    private void writeConnectionSettings( Bundle b )
    {
        b.putBoolean("autoconnect", mAutoConnect );
    }

    /**
     * Helper function for reading connection settings from bundle data
     * @param b	Bundle data to read from
     */
    private void readConnectionSettings( Bundle b )
    {
        mAutoConnect = b.getBoolean( "autoconnect" );
    }
}
