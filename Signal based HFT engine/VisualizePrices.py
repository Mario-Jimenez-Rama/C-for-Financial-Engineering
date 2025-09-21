import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
from datetime import datetime
import os

# Set style for better visuals
plt.style.use('seaborn-v0_8')
sns.set_palette("husl")

def save_plot(fig, plot_name, formats=['pdf']):
    """Save plot in PDF format only to a specific folder"""
    # Create a dedicated plots folder
    plots_dir = "saved_plots"
    os.makedirs(plots_dir, exist_ok=True)  # Create folder if it doesn't exist
    
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    base_name = f"{plot_name}_{timestamp}"
    
    for fmt in formats:
        filename = os.path.join(plots_dir, f"{base_name}.{fmt}")
        fig.savefig(filename, dpi=300, bbox_inches='tight', format=fmt)
        print(f"💾 Saved PDF: {filename}")
        print(f"📁 Location: {os.path.abspath(filename)}")
    
    return base_name

def debug_file_locations():
    """Debug function to see where files are being saved"""
    print("\n🔍 Debug File Locations:")
    print(f"Current working directory: {os.getcwd()}")
    print(f"Script directory: {os.path.dirname(os.path.abspath(__file__))}")
    
    # Check if plots folder exists
    plots_dir = "saved_plots"
    if os.path.exists(plots_dir):
        print(f"📁 Plots directory exists: {os.path.abspath(plots_dir)}")
        pdf_files = [f for f in os.listdir(plots_dir) if f.endswith('.pdf')]
        if pdf_files:
            print("📄 PDF files found:")
            for pdf in pdf_files:
                full_path = os.path.join(plots_dir, pdf)
                print(f"   - {pdf} ({os.path.getsize(full_path)} bytes)")
        else:
            print("❌ No PDF files in plots directory")
    else:
        print("❌ Plots directory does not exist yet")

def create_order_history_plots(csv_file):
    """Create order history plots and return figure"""
    print("📊 Loading order history...")
    try:
        df_orders = pd.read_csv(csv_file)
        print(f"✅ Loaded {len(df_orders)} orders")
        
        # Convert timestamp to datetime for better plotting
        df_orders['timestamp'] = pd.to_datetime(df_orders['timestamp_ns'], unit='ns')
        
        # Create subplots
        fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(15, 12))
        fig.suptitle('Trading Order Analysis', fontsize=16, fontweight='bold')
        
        # Plot 1: Orders by instrument
        instrument_counts = df_orders['instrument_id'].value_counts().sort_index()
        ax1.bar(instrument_counts.index.astype(str), instrument_counts.values)
        ax1.set_title('Orders per Instrument')
        ax1.set_xlabel('Instrument ID')
        ax1.set_ylabel('Number of Orders')
        ax1.tick_params(axis='x', rotation=45)
        
        # Plot 2: Buy vs Sell distribution
        side_counts = df_orders['side'].value_counts()
        ax2.pie(side_counts.values, labels=side_counts.index, autopct='%1.1f%%')
        ax2.set_title('Buy vs Sell Orders')
        
        # Plot 3: Price distribution
        ax3.hist(df_orders['price'], bins=30, alpha=0.7, edgecolor='black')
        ax3.set_title('Order Price Distribution')
        ax3.set_xlabel('Price')
        ax3.set_ylabel('Frequency')
        
        # Plot 4: Orders over time
        time_grouped = df_orders.groupby(df_orders['timestamp'].dt.floor('S')).size()
        ax4.plot(time_grouped.index, time_grouped.values, marker='o', linestyle='-', markersize=2)
        ax4.set_title('Orders Over Time')
        ax4.set_xlabel('Time')
        ax4.set_ylabel('Orders per Second')
        ax4.tick_params(axis='x', rotation=45)
        
        plt.tight_layout()
        
        # Additional statistics
        print("\n📈 Order Statistics:")
        print(f"Total Orders: {len(df_orders)}")
        print(f"Buy Orders: {len(df_orders[df_orders['side'] == 'BUY'])}")
        print(f"Sell Orders: {len(df_orders[df_orders['side'] == 'SELL'])}")
        print(f"Average Price: ${df_orders['price'].mean():.2f}")
        print(f"Price Range: ${df_orders['price'].min():.2f} - ${df_orders['price'].max():.2f}")
        
        return fig
        
    except FileNotFoundError:
        print(f"❌ File {csv_file} not found. Run C++ program first to generate data.")
        return None
    except Exception as e:
        print(f"❌ Error loading order history: {e}")
        return None

def create_price_analysis_plots(csv_file, instrument_id):
    """Create price analysis plots and return figure"""
    print(f"📈 Loading price data for instrument {instrument_id}...")
    try:
        df_prices = pd.read_csv(csv_file)
        print(f"✅ Loaded {len(df_prices)} price points")
        
        # Create subplots
        fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(15, 12))
        fig.suptitle(f'Price Analysis - Instrument {instrument_id}', fontsize=16, fontweight='bold')
        
        # Plot 1: Price timeline
        ax1.plot(df_prices['timestamp_ns'], df_prices['price'], linewidth=1)
        ax1.set_title('Price Movement')
        ax1.set_xlabel('Time (ns)')
        ax1.set_ylabel('Price')
        ax1.grid(True, alpha=0.3)
        
        # Plot 2: Histogram of prices
        ax2.hist(df_prices['price'], bins=20, alpha=0.7, edgecolor='black')
        ax2.set_title('Price Distribution')
        ax2.set_xlabel('Price')
        ax2.set_ylabel('Frequency')
        
        # Plot 3: Moving averages
        df_prices['MA5'] = df_prices['price'].rolling(window=5).mean()
        df_prices['MA10'] = df_prices['price'].rolling(window=10).mean()
        ax3.plot(df_prices['timestamp_ns'], df_prices['price'], label='Price', linewidth=1, alpha=0.7)
        ax3.plot(df_prices['timestamp_ns'], df_prices['MA5'], label='5-Point MA', linewidth=2)
        ax3.plot(df_prices['timestamp_ns'], df_prices['MA10'], label='10-Point MA', linewidth=2)
        ax3.set_title('Price with Moving Averages')
        ax3.set_xlabel('Time (ns)')
        ax3.set_ylabel('Price')
        ax3.legend()
        ax3.grid(True, alpha=0.3)
        
        # Plot 4: Returns volatility
        returns = df_prices['price'].pct_change().dropna()
        ax4.plot(returns.index, returns.values, linewidth=1, alpha=0.7)
        ax4.axhline(y=returns.mean(), color='r', linestyle='--', label=f'Mean: {returns.mean():.4f}')
        ax4.axhline(y=returns.std(), color='g', linestyle='--', label=f'Std Dev: {returns.std():.4f}')
        ax4.axhline(y=-returns.std(), color='g', linestyle='--')
        ax4.set_title('Price Returns (%)')
        ax4.set_xlabel('Time Index')
        ax4.set_ylabel('Daily Return')
        ax4.legend()
        ax4.grid(True, alpha=0.3)
        
        plt.tight_layout()
        
        # Calculate statistics
        price_stats = df_prices['price'].describe()
        volatility = returns.std() * np.sqrt(252)  # Annualized volatility (approx)
        
        print(f"\n📊 Price Statistics for Instrument {instrument_id}:")
        print(f"Total Price Points: {len(df_prices)}")
        print(f"Average Price: ${price_stats['mean']:.2f}")
        print(f"Minimum Price: ${price_stats['min']:.2f}")
        print(f"Maximum Price: ${price_stats['max']:.2f}")
        print(f"Price Volatility: {volatility:.2%} (annualized)")
        print(f"Price Range: ${price_stats['max'] - price_stats['min']:.2f}")
        
        return fig
        
    except FileNotFoundError:
        print(f"❌ File {csv_file} not found. Run C++ program first to generate data.")
        return None
    except Exception as e:
        print(f"❌ Error loading price data: {e}")
        return None

def create_comprehensive_dashboard():
    """Create comprehensive dashboard and return figure"""
    try:
        # Load both files
        df_orders = pd.read_csv('order_history.csv')
        df_prices = pd.read_csv('price_data_instrument_0.csv')
        
        fig = plt.figure(figsize=(16, 10))
        
        # Order side distribution
        ax1 = plt.subplot2grid((3, 3), (0, 0))
        side_counts = df_orders['side'].value_counts()
        ax1.pie(side_counts.values, labels=side_counts.index, autopct='%1.1f%%')
        ax1.set_title('Order Side Distribution')
        
        # Price movement
        ax2 = plt.subplot2grid((3, 3), (0, 1), colspan=2)
        ax2.plot(df_prices['timestamp_ns'], df_prices['price'], linewidth=2)
        ax2.set_title('Price Movement - Instrument 0')
        ax2.set_ylabel('Price')
        ax2.grid(True, alpha=0.3)
        
        # Instrument distribution
        ax3 = plt.subplot2grid((3, 3), (1, 0))
        instrument_counts = df_orders['instrument_id'].value_counts().sort_index()
        ax3.bar(instrument_counts.index.astype(str), instrument_counts.values)
        ax3.set_title('Orders per Instrument')
        ax3.tick_params(axis='x', rotation=45)
        
        # Price histogram
        ax4 = plt.subplot2grid((3, 3), (1, 1))
        ax4.hist(df_orders['price'], bins=25, alpha=0.7, edgecolor='black')
        ax4.set_title('Order Price Distribution')
        ax4.set_xlabel('Price')
        
        # Returns distribution
        ax5 = plt.subplot2grid((3, 3), (1, 2))
        returns = df_prices['price'].pct_change().dropna()
        ax5.hist(returns, bins=20, alpha=0.7, edgecolor='black')
        ax5.set_title('Price Returns Distribution')
        ax5.set_xlabel('Return')
        
        # Correlation heatmap (if multiple instruments)
        ax6 = plt.subplot2grid((3, 3), (2, 0), colspan=3)
        ax6.text(0.5, 0.5, 'Correlation Analysis Area\n(Expand with more instruments)', 
                ha='center', va='center', transform=ax6.transAxes, fontsize=12)
        ax6.set_title('Market Correlation Analysis')
        ax6.axis('off')
        
        plt.tight_layout()
        return fig
        
    except FileNotFoundError:
        print("❌ Required CSV files not found. Run C++ program first.")
        return None
    except Exception as e:
        print(f"❌ Error creating dashboard: {e}")
        return None
    
def main():
    """Main function to run all visualizations"""
    print(f"Current working directory: {os.getcwd()}")
    print(f"Files in directory: {os.listdir('.')}")
    print("🎯 Trading Data Visualization Tool")
    print("=" * 50)
    
    # Create plots directory upfront
    plots_dir = "saved_plots"
    os.makedirs(plots_dir, exist_ok=True)
    print(f"📁 PDFs will be saved in: {os.path.abspath(plots_dir)}")
    
    while True:
        print("\n📋 Menu Options:")
        print("1. Visualize Order History (with save)")
        print("2. Visualize Price Data (Instrument 0, with save)")
        print("3. Comprehensive Dashboard (with save)")
        print("4. Debug file locations")
        print("5. Open PDFs folder")
        print("6. Exit")
        
        choice = input("\nEnter your choice (1-6): ").strip()
        
        if choice == '1':
            fig = create_order_history_plots('order_history.csv')
            if fig:
                save_plot(fig, "order_history_analysis")
                plt.show()
                
        elif choice == '2':
            fig = create_price_analysis_plots('price_data_instrument_0.csv', 0)
            if fig:
                save_plot(fig, "price_analysis_instrument_0")
                plt.show()
                
        elif choice == '3':
            fig = create_comprehensive_dashboard()
            if fig:
                save_plot(fig, "trading_dashboard")
                plt.show()
                
        elif choice == '4':
            debug_file_locations()
            
        elif choice == '5':
            # Open the plots folder
            plots_path = os.path.abspath("saved_plots")
            try:
                if os.name == 'nt':  # Windows
                    os.system(f'explorer "{plots_path}"')
                elif os.name == 'posix':  # Mac
                    os.system(f'open "{plots_path}"')
                elif os.name == 'linux':  # Linux
                    os.system(f'nautilus "{plots_path}"')
                print(f"📂 Opened folder: {plots_path}")
            except Exception as e:
                print(f"❌ Could not open folder: {e}")
                
        elif choice == '6':
            print("👋 Goodbye!")
            break
            
        else:
            print("❌ Invalid choice. Please try again.")
        
        input("\nPress Enter to continue...")

if __name__ == "__main__":
    main()