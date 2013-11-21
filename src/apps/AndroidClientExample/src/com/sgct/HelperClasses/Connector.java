package com.sgct.HelperClasses;

import java.io.PrintWriter;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.UnknownHostException;

/**
 * Helper singleton class for connecting to a server and sending/receiving messages.
 * Created as a singleton as it should be consistent through different activities.
 * User: alefr38
 * Date: 2012-02-22
 */
public class Connector {

    // Singleton declarations
    private static final Connector instance = new Connector();

    public static Connector getInstance() {
        return instance;
    }

    //------------------------------------------------------------------------------//
    // Class variable declaration
    //------------------------------------------------------------------------------//

    /** For assigning and comparison if no ip is defined for certain submask */
    public static final String NO_IP_DEFINED = "-1";

    /*! Socket used for communication */
    private Socket mSocket = null;

    /*! Connection address */
    private InetSocketAddress mAddress = new InetSocketAddress("0.0.0.0", 0);

    /*! Contains error message if something goes wrong */
    private String mErrorMsg = "";

    //------------------------------------------------------------------------------//
    // Constructors
    //------------------------------------------------------------------------------//

    /**
     * Default constructor
     */
    private Connector()
    {
        ; // Do nothing
    }

    //------------------------------------------------------------------------------//
    // Class functions
    //------------------------------------------------------------------------------//

     /**
     * Send data to the host connected on the socket.
     * @param   data    Data to send to server
     * @return If sent correctly or not
     */
    public boolean send( String data ){
        if( !isConnected() ){
            mErrorMsg = "Not connected, could not send data";
            return false;
        }

        PrintWriter out;
        try{
            out = new PrintWriter( mSocket.getOutputStream(), false );
        } catch( Exception e ) {
            mErrorMsg = e.getLocalizedMessage();
            return false;
        }

        out.print( data + "\r\n" );
        out.flush();
        //out.close();
        
        return true;
    }

    /**
     * Try connecting to the given ip and port.
     * @param ip	    Ip-address of server to connect to.
     * @param port	    Port number to use for connection.
     * @param timeout   Timeout length in milliseconds, a value of 0 will wait until connected
     * @return	If connected correctly
     */
    public boolean connect( String ip, int port, int timeout ){
        try {
            InetAddress address = InetAddress.getByName( ip );
            return connect( new InetSocketAddress( address, port ), timeout ) ;
        }
        catch (UnknownHostException e) {
            mErrorMsg = e.getLocalizedMessage();
            return false;
        }
    }

    /**
     * Try connecting to the specified address
     * @param host   Address with valid host address and port
     * @param timeout   Timeout length in milliseconds, a value of 0 will wait until connected
     * @return	If connected correctly
     */
    public boolean connect( InetSocketAddress host, int timeout ){

        disconnect();   // disconnect won't do anything if not connected

        try {
            mAddress = host;
            mSocket = new Socket();
            mSocket.connect( mAddress, timeout );
        } catch (Exception e ) {
            mSocket = null;
            mErrorMsg = e.getLocalizedMessage();
            return false;
        }

        return true;
    }

    /**
     * Disconnects the Connection if it is connected
     * @return If disconnected correctly (also false if wasn't already connected to a host)
     */
    public boolean disconnect(){
        if( !isConnected() ){
            return false;
        }

        try{
            mSocket.close();
            mSocket = null;
        } catch( Exception e ) {
            mErrorMsg = e.getLocalizedMessage();
        }

        return true;
    }

    /**
     * Check if the Connector is connected
     * @return True if connected to a server, otherwise false
     */
    public boolean isConnected()
    {
        return mSocket != null && mSocket.isConnected();
    }

    /**
     * Get the error message if anything has gone wrong.
     * @return Current error message
     */
    public String getErrorMessage()
    {
        return mErrorMsg;
    }

    /**
     * Get the address that the connector is -, was last- or has tried connecting to
     * @return  Address that the connector is -, was last- or has tried connecting to
     */
    public String getAddress(){
        return mAddress.getAddress().getHostAddress().toString();
    }

    /**
     * Get the port number that the connector is -, was last- or has tried connecting to
     * @return  Port number that the connector is -, was last- or has tried connecting to
     */
    public int getPort(){
        return mAddress.getPort();
    }
}
